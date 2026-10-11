"""Interactive 2-D inspector for Run3 semi-infinite block worlds.

The generator in this file intentionally mirrors SeededBlockGenerator in
source/gameplay/SemiInfiniteWorld.cpp. The canvas is virtual: only visible
cells are generated, so the complete 1000 km logical world is never allocated.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import math
from pathlib import Path
import sys
from typing import Iterable


MASK64 = (1 << 64) - 1
STANDARD_CELL_CM = 2000.0
BIOME_SALT = 0x42494F4D45
TILE_SALT = 0x54494C45
OCCUPANCY_SALT = 0x4F43435550414E43
VARIANT_SALT = 0x56415249414E5400


def u64(value: int) -> int:
    return value & MASK64


def mix64(value: int) -> int:
    value = u64(value + 0x9E3779B97F4A7C15)
    value = u64((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9)
    value = u64((value ^ (value >> 27)) * 0x94D049BB133111EB)
    return u64(value ^ (value >> 31))


def coordinate_hash(seed: int, x: int, z: int, salt: int = 0) -> int:
    value = u64(seed) ^ u64(salt)
    value ^= mix64(u64(x))
    value ^= mix64(u64(u64(z) + 0x632BE59BD9B4E019))
    return mix64(value)


@dataclass(frozen=True)
class Port:
    side: str
    position_cm: tuple[float, float, float]
    size_cm: tuple[float, float]


@dataclass(frozen=True)
class Block:
    identifier: str
    mesh: str
    style: str
    dimensions_cm: tuple[float, float, float]
    ports: tuple[Port, ...]
    termination: bool = False

    @property
    def is_runtime_tile(self) -> bool:
        return (
            not self.termination
            and abs(self.dimensions_cm[0] - STANDARD_CELL_CM) < 0.01
            and abs(self.dimensions_cm[1] - STANDARD_CELL_CM) < 0.01
        )


@dataclass(frozen=True)
class Placement:
    anchor: tuple[int, int]
    block: Block
    width_cells: int
    depth_cells: int
    root_cm: tuple[float, float, float]


@dataclass(frozen=True)
class Manifest:
    path: Path
    units_per_meter: int
    grid_cm: int
    biome_scale_cm: float
    blocks: tuple[Block, ...]

    @classmethod
    def load(cls, path: Path) -> "Manifest":
        data = json.loads(path.read_text(encoding="utf-8"))
        blocks: list[Block] = []
        identifiers: set[str] = set()
        for entry in data.get("blocks", []):
            identifier = str(entry["id"])
            if not identifier or identifier in identifiers:
                raise ValueError(f"duplicate or empty block id: {identifier!r}")
            identifiers.add(identifier)
            dimensions = tuple(float(value) for value in entry["dimensions_cm"])
            if len(dimensions) != 3 or min(dimensions) <= 0:
                raise ValueError(f"invalid dimensions for {identifier}")
            ports = []
            for source in entry.get("ports", []):
                position = tuple(float(value) for value in source["position_cm"])
                size = tuple(float(value) for value in source.get("size_cm", (400, 320)))
                if source["side"] not in {"N", "S", "E", "W"}:
                    raise ValueError(f"invalid port side in {identifier}")
                if len(position) != 3 or len(size) != 2:
                    raise ValueError(f"invalid port geometry in {identifier}")
                ports.append(Port(source["side"], position, size))
            block = Block(
                identifier,
                str(entry.get("mesh", identifier + ".mesh")),
                str(entry.get("style", "DEFAULT")),
                dimensions,
                tuple(ports),
                bool(entry.get("termination", False)),
            )
            blocks.append(block)
            for source in entry.get("variants", []):
                variant_id = str(source["id"])
                if not variant_id or variant_id in identifiers:
                    raise ValueError(f"duplicate or empty block variant: {variant_id!r}")
                identifiers.add(variant_id)
                open_sides = set(str(source.get("open_sides", "NSEW")))
                blocks.append(Block(
                    variant_id,
                    str(source.get("mesh", variant_id + ".mesh")),
                    block.style,
                    block.dimensions_cm,
                    tuple(port for port in block.ports if port.side in open_sides),
                    block.termination,
                ))
        units = int(data.get("units_per_meter", 100))
        grid = int(data.get("grid_cm", 500))
        biome = float(data.get("biome_scale_cm", 24000.0))
        if not blocks or units <= 0 or grid <= 0 or biome < STANDARD_CELL_CM:
            raise ValueError("manifest has invalid scale constants or no blocks")
        return cls(path.resolve(), units, grid, biome, tuple(blocks))


@dataclass(frozen=True)
class WorldConfig:
    path: Path
    seed: int
    manifest_path: Path
    origin_block: str
    load_radius_cm: float
    vertical_radius_cm: float
    extent_cm: int
    light_budget: int

    @classmethod
    def load(cls, path: Path, content_root: Path | None = None) -> "WorldConfig":
        values: dict[str, str] = {}
        for raw in path.read_text(encoding="utf-8-sig").splitlines():
            line = raw.strip()
            if not line or line.startswith(("#", ";")) or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
        if "WorldSeed" not in values:
            raise ValueError(f"{path} does not define WorldSeed")
        seed = int(values["WorldSeed"], 10)
        if not 0 <= seed <= MASK64:
            raise ValueError("WorldSeed must be an unsigned 64-bit integer")
        manifest_value = values.get("BlockManifest", "run3/bblocks/block_manifest.json")
        if content_root is not None:
            manifest = content_root / manifest_value
        else:
            # The standard overlay layout is .../run3/maps/<quality>/<map>/scene.cfg.
            run3 = next((parent for parent in path.parents if parent.name.lower() == "run3"), None)
            manifest = run3 / Path(manifest_value).relative_to("run3") if run3 else path.parent / manifest_value
        return cls(
            path.resolve(), seed, manifest.resolve(),
            values.get("WorldOriginBlock", ""),
            float(values.get("WorldLoadRadius", "20000")),
            float(values.get("WorldVerticalRadius", "3000")),
            int(values.get("WorldExtent", "100000000")),
            int(values.get("WorldLightBudget", "3")),
        )


class SeededWorld:
    def __init__(self, manifest: Manifest, seed: int, origin_block: str = "") -> None:
        if not 0 <= seed <= MASK64:
            raise ValueError("seed must fit uint64_t")
        self.manifest = manifest
        self.seed = seed
        live = tuple(block for block in manifest.blocks if not block.termination)
        self.small = tuple(block for block in live if block.dimensions_cm[:2] == (2000.0, 2000.0))
        self.large = tuple(block for block in live if block.dimensions_cm[:2] == (4000.0, 4000.0))
        self.houses = tuple(block for block in live if block.dimensions_cm[:2] == (12000.0, 8000.0))
        if not self.small or not self.large or not self.houses:
            raise ValueError("manifest needs 20m, 40m and 120x80m occupancy blocks")
        self.tiles = live
        self.styles = tuple(dict.fromkeys(block.style for block in self.small + self.large))
        self.origin = self.small[0]
        if origin_block:
            matches = [block for block in self.small if block.identifier == origin_block]
            if not matches:
                raise ValueError("origin block must be a 20 metre occupancy tile")
            self.origin = matches[0]
        # C++ std::llround for this positive value is floor(v + 0.5).
        self.biome_span = max(1, math.floor(manifest.biome_scale_cm / STANDARD_CELL_CM + 0.5))

    def biome(self, x: int, z: int) -> str:
        coarse_x = x // self.biome_span
        coarse_z = z // self.biome_span
        index = coordinate_hash(self.seed, coarse_x, coarse_z, BIOME_SALT) % len(self.styles)
        return self.styles[index]

    def block(self, x: int, z: int) -> Block:
        return self.placement(x, z).block

    def _select(self, source: tuple[Block, ...], style: str, x: int, z: int,
                salt: int) -> Block:
        choices = tuple(block for block in source if block.style == style) or source
        return choices[coordinate_hash(self.seed, x, z, salt) % len(choices)]

    def placement(self, x: int, z: int) -> Placement:
        macro_x, macro_z = x // 6, z // 4
        minimum_x, minimum_z = macro_x * 6, macro_z * 4
        origin_macro = macro_x == 0 and macro_z == 0
        use_house = (not origin_macro and
                     coordinate_hash(self.seed, macro_x, macro_z, OCCUPANCY_SALT) % 11 == 0)
        if use_house:
            block = self._select(self.houses, "HOUSE70", macro_x, macro_z, VARIANT_SALT)
            return self._make_placement(minimum_x, minimum_z, block, 6, 4)
        slot_x, slot_z = (x - minimum_x) // 2, (z - minimum_z) // 2
        anchor_x, anchor_z = minimum_x + slot_x * 2, minimum_z + slot_z * 2
        use_large = (not origin_macro and
                     coordinate_hash(self.seed, anchor_x, anchor_z,
                                     OCCUPANCY_SALT ^ 0x40) % 100 < 48)
        if use_large:
            block = self._select(self.large, self.biome(anchor_x, anchor_z),
                                 anchor_x, anchor_z, VARIANT_SALT ^ 0x40)
            return self._make_placement(anchor_x, anchor_z, block, 2, 2)
        block = (self.origin if (x, z) == (0, 0) else
                 self._select(self.small, self.biome(x, z), x, z, VARIANT_SALT))
        return self._make_placement(x, z, block, 1, 1)

    @staticmethod
    def _make_placement(x: int, z: int, block: Block,
                        width: int, depth: int) -> Placement:
        root = ((x + (width - 1) * 0.5) * STANDARD_CELL_CM, 0.0,
                (z + (depth - 1) * 0.5) * STANDARD_CELL_CM)
        return Placement((x, z), block, width, depth, root)

    @staticmethod
    def _socket(placement: Placement, port: Port) -> tuple[int, int, int, str]:
        # Authoring X/depth/Z-up becomes Ogre X/Y-up/-Z.
        return (round(placement.root_cm[0] + port.position_cm[0]),
                round(placement.root_cm[1] + port.position_cm[2]),
                round(placement.root_cm[2] - port.position_cm[1]), port.side)

    def port_is_mated(self, placement: Placement, port: Port) -> bool:
        x, y, z, side = self._socket(placement, port)
        opposite = {"N": "S", "S": "N", "E": "W", "W": "E"}[side]
        centre_x = round(x / STANDARD_CELL_CM)
        centre_z = round(z / STANDARD_CELL_CM)
        candidates: dict[tuple[int, int], Placement] = {}
        for cell_z in range(centre_z - 4, centre_z + 5):
            for cell_x in range(centre_x - 4, centre_x + 5):
                candidate = self.placement(cell_x, cell_z)
                candidates[candidate.anchor] = candidate
        return any(
            candidate.anchor != placement.anchor and
            any(self._socket(candidate, other) == (x, y, z, opposite)
                for other in candidate.block.ports)
            for candidate in candidates.values()
        )

    def inspect(self, x: int, z: int) -> dict[str, object]:
        placement = self.placement(x, z)
        block = placement.block
        return {
            "cell": [x, z],
            "world_cm": [x * STANDARD_CELL_CM, z * STANDARD_CELL_CM],
            "world_m": [x * 20, z * 20],
            "biome": self.biome(x, z),
            "placement_anchor": list(placement.anchor),
            "footprint_cells": [placement.width_cells, placement.depth_cells],
            "root_cm": list(placement.root_cm),
            "block": block.identifier,
            "mesh": block.mesh,
            "dimensions_cm": list(block.dimensions_cm),
            "ports": [{"side": port.side,
                       "mated": self.port_is_mated(placement, port)}
                      for port in block.ports],
        }


def default_scene() -> Path:
    return (
        Path(__file__).resolve().parents[1]
        / "derived-content/luniverse/run3/maps/high/luniverse/scene.cfg"
    )


def create_world(scene: Path, content_root: Path | None = None) -> tuple[WorldConfig, SeededWorld]:
    config = WorldConfig.load(scene, content_root)
    manifest = Manifest.load(config.manifest_path)
    return config, SeededWorld(manifest, config.seed, config.origin_block)


def dump_region(world: SeededWorld, centre_x: int, centre_z: int, radius: int) -> None:
    result = []
    for z in range(centre_z - radius, centre_z + radius + 1):
        result.append([world.inspect(x, z) for x in range(centre_x - radius, centre_x + radius + 1)])
    print(json.dumps(result, indent=2))


def verify_runtime_vectors(manifest: Manifest) -> None:
    """Validate C++ parity, complete occupancy, and manifest mesh reachability."""
    world = SeededWorld(manifest, 0x123456789ABCDEF0, "01_BACKROOMS_20")
    expected = {
        (-1, 0): "03_POOLROOMS_20_Drained_NS",
        (12, 12): "04_POOLROOMS_40_OpenIsland_EW",
        (137, -91): "02_BACKROOMS_40_Atrium_EW",
    }
    actual = {cell: world.block(*cell).identifier for cell in expected}
    if actual != expected:
        raise RuntimeError(f"runtime parity vectors failed: {actual!r}")

    generated_meshes: set[str] = set()
    for z in range(-40, 41):
        for x in range(-40, 41):
            placement = world.placement(x, z)
            anchor_x, anchor_z = placement.anchor
            if not (anchor_x <= x < anchor_x + placement.width_cells and
                    anchor_z <= z < anchor_z + placement.depth_cells):
                raise RuntimeError(
                    f"cell {(x, z)} is outside planned footprint {placement!r}")
            repeated = world.placement(anchor_x, anchor_z)
            if repeated != placement:
                raise RuntimeError(
                    f"placement is not stable at anchor {placement.anchor}")
            generated_meshes.add(placement.block.mesh)
    expected_meshes = {block.mesh for block in world.tiles}
    if generated_meshes != expected_meshes:
        missing = sorted(expected_meshes - generated_meshes)
        raise RuntimeError(f"manifest meshes not reachable by planner: {missing!r}")


def run_gui(config: WorldConfig, initial_world: SeededWorld,
            quit_after_ms: int = 0) -> int:
    try:
        from PyQt5.QtCore import QPoint, QPointF, QRectF, Qt, QTimer
        from PyQt5.QtGui import QColor, QFont, QImage, QPainter, QPen
        from PyQt5.QtWidgets import (
            QAction, QApplication, QFileDialog, QFormLayout, QHBoxLayout,
            QLabel, QLineEdit, QMainWindow, QMessageBox, QPushButton,
            QSpinBox, QSplitter, QStatusBar, QTextEdit, QToolBar, QVBoxLayout,
            QWidget,
        )
    except ImportError as error:
        raise RuntimeError("PyQt5 is required: python -m pip install PyQt5") from error

    palette = (
        QColor("#c8a94b"), QColor("#318fa4"), QColor("#a47ac2"),
        QColor("#739b54"), QColor("#ba7652"), QColor("#667fba"),
        QColor("#9b6d72"), QColor("#4e9b81"),
    )

    class MapCanvas(QWidget):
        def __init__(self, world: SeededWorld, cfg: WorldConfig) -> None:
            super().__init__()
            self.world = world
            self.config = cfg
            self.centre = QPointF(0.0, 0.0)
            self.pixels_per_cell = 34.0
            self.drag_at: QPoint | None = None
            self.drag_centre = QPointF()
            self.selected = (0, 0)
            self.on_selection = lambda _info: None
            self.setFocusPolicy(Qt.StrongFocus)
            self.setMouseTracking(True)
            self.setMinimumSize(500, 400)

        def set_world(self, world: SeededWorld, cfg: WorldConfig) -> None:
            self.world, self.config = world, cfg
            self.selected = (0, 0)
            self.centre = QPointF(0.0, 0.0)
            self.on_selection(world.inspect(0, 0))
            self.update()

        def screen_for_cell(self, x: float, z: float) -> QPointF:
            return QPointF(
                self.width() * 0.5 + (x - self.centre.x()) * self.pixels_per_cell,
                self.height() * 0.5 + (z - self.centre.y()) * self.pixels_per_cell,
            )

        def cell_for_screen(self, point: QPointF) -> QPointF:
            return QPointF(
                self.centre.x() + (point.x() - self.width() * 0.5) / self.pixels_per_cell,
                self.centre.y() + (point.y() - self.height() * 0.5) / self.pixels_per_cell,
            )

        def style_colour(self, style: str) -> QColor:
            try:
                return palette[self.world.styles.index(style) % len(palette)]
            except ValueError:
                return QColor("#777777")

        def paintEvent(self, _event) -> None:
            painter = QPainter(self)
            painter.fillRect(self.rect(), QColor("#171a1f"))
            painter.setRenderHint(QPainter.Antialiasing, False)
            half_x = self.width() / self.pixels_per_cell / 2 + 2
            half_z = self.height() / self.pixels_per_cell / 2 + 2
            minimum_x = math.floor(self.centre.x() - half_x)
            maximum_x = math.ceil(self.centre.x() + half_x)
            minimum_z = math.floor(self.centre.y() - half_z)
            maximum_z = math.ceil(self.centre.y() + half_z)
            maximum_cell = (self.config.extent_cm // 2) // int(STANDARD_CELL_CM)
            grid_pen = QPen(QColor(20, 23, 28, 145), 1)
            label_font = QFont("Consolas", 8)
            placements: dict[tuple[int, int], Placement] = {}
            for z in range(minimum_z, maximum_z + 1):
                for x in range(minimum_x, maximum_x + 1):
                    top_left = self.screen_for_cell(x - 0.5, z - 0.5)
                    rect = QRectF(top_left.x(), top_left.y(), self.pixels_per_cell, self.pixels_per_cell)
                    if abs(x) > maximum_cell or abs(z) > maximum_cell:
                        painter.fillRect(rect, QColor("#090a0c"))
                        continue
                    placement = self.world.placement(x, z)
                    placements[placement.anchor] = placement
                    painter.setPen(grid_pen)
                    painter.drawRect(rect)
            for placement in placements.values():
                block = placement.block
                top_left = self.screen_for_cell(placement.anchor[0] - 0.5,
                                                placement.anchor[1] - 0.5)
                rect = QRectF(top_left.x(), top_left.y(),
                              self.pixels_per_cell * placement.width_cells,
                              self.pixels_per_cell * placement.depth_cells)
                colour = self.style_colour(block.style)
                # Stable per-block tint, useful when a biome has variants.
                tint = int(coordinate_hash(
                    0, len(block.identifier), sum(map(ord, block.identifier))) % 24) - 12
                colour = (colour.lighter(100 + tint) if tint >= 0
                          else colour.darker(100 - tint))
                painter.fillRect(rect, colour)
                painter.setPen(QPen(QColor("#11151a"), 2))
                painter.drawRect(rect)
                if self.pixels_per_cell >= 28 or placement.width_cells > 1:
                    painter.setFont(label_font)
                    painter.setPen(QColor("#101216"))
                    painter.drawText(rect.adjusted(3, 2, -2, -2),
                                     Qt.AlignLeft | Qt.AlignTop,
                                     block.identifier.replace("_", " "))
                if self.pixels_per_cell >= 10:
                    for port in block.ports:
                        socket_x, _socket_y, socket_z, _side = self.world._socket(
                            placement, port)
                        point = self.screen_for_cell(socket_x / STANDARD_CELL_CM,
                                                     socket_z / STANDARD_CELL_CM)
                        mated = self.world.port_is_mated(placement, port)
                        colour = QColor("#3cff88") if mated else QColor("#ff344f")
                        painter.setPen(QPen(colour, 3))
                        painter.drawEllipse(point, 3.0, 3.0)
            # World origin.
            origin = self.screen_for_cell(0, 0)
            painter.setPen(QPen(QColor("#ff4d67"), 2))
            painter.drawLine(int(origin.x() - 8), int(origin.y()), int(origin.x() + 8), int(origin.y()))
            painter.drawLine(int(origin.x()), int(origin.y() - 8), int(origin.x()), int(origin.y() + 8))
            # Runtime streaming radius around the selected/navigation position.
            selected = self.screen_for_cell(*self.selected)
            radius = self.config.load_radius_cm / STANDARD_CELL_CM * self.pixels_per_cell
            painter.setPen(QPen(QColor(255, 255, 255, 150), 2, Qt.DashLine))
            painter.drawEllipse(selected, radius, radius)
            chosen = self.world.placement(*self.selected)
            chosen_top_left = self.screen_for_cell(chosen.anchor[0] - 0.5,
                                                   chosen.anchor[1] - 0.5)
            painter.setPen(QPen(QColor("#ffffff"), 3))
            painter.drawRect(QRectF(chosen_top_left.x(), chosen_top_left.y(),
                                    chosen.width_cells * self.pixels_per_cell,
                                    chosen.depth_cells * self.pixels_per_cell))

        def mousePressEvent(self, event) -> None:
            if event.button() in (Qt.MiddleButton, Qt.RightButton):
                self.drag_at = event.pos()
                self.drag_centre = QPointF(self.centre)
                self.setCursor(Qt.ClosedHandCursor)
            elif event.button() == Qt.LeftButton:
                cell = self.cell_for_screen(event.localPos())
                self.selected = (math.floor(cell.x() + 0.5), math.floor(cell.y() + 0.5))
                self.on_selection(self.world.inspect(*self.selected))
                self.update()

        def mouseMoveEvent(self, event) -> None:
            if self.drag_at is not None:
                delta = event.pos() - self.drag_at
                self.centre = QPointF(
                    self.drag_centre.x() - delta.x() / self.pixels_per_cell,
                    self.drag_centre.y() - delta.y() / self.pixels_per_cell,
                )
                self.update()

        def mouseReleaseEvent(self, event) -> None:
            if event.button() in (Qt.MiddleButton, Qt.RightButton):
                self.drag_at = None
                self.unsetCursor()

        def wheelEvent(self, event) -> None:
            before = self.cell_for_screen(event.pos())
            factor = 1.25 if event.angleDelta().y() > 0 else 0.8
            self.pixels_per_cell = min(120.0, max(5.0, self.pixels_per_cell * factor))
            after = self.cell_for_screen(event.pos())
            self.centre += before - after
            self.update()

        def keyPressEvent(self, event) -> None:
            amount = max(1.0, 350.0 / self.pixels_per_cell)
            if event.key() in (Qt.Key_Left, Qt.Key_A): self.centre.setX(self.centre.x() - amount)
            elif event.key() in (Qt.Key_Right, Qt.Key_D): self.centre.setX(self.centre.x() + amount)
            elif event.key() in (Qt.Key_Up, Qt.Key_W): self.centre.setY(self.centre.y() - amount)
            elif event.key() in (Qt.Key_Down, Qt.Key_S): self.centre.setY(self.centre.y() + amount)
            elif event.key() == Qt.Key_Home: self.centre = QPointF(0.0, 0.0)
            else:
                super().keyPressEvent(event)
                return
            self.update()

        def jump(self, x: int, z: int) -> None:
            self.centre = QPointF(float(x), float(z))
            self.selected = (x, z)
            self.on_selection(self.world.inspect(x, z))
            self.update()

    class Window(QMainWindow):
        def __init__(self, cfg: WorldConfig, world: SeededWorld) -> None:
            super().__init__()
            self.config, self.world = cfg, world
            self.setWindowTitle("Run3 Luniverse Seed Map")
            self.resize(1280, 820)
            self.canvas = MapCanvas(world, cfg)
            self.details = QTextEdit()
            self.details.setReadOnly(True)
            self.details.setFont(QFont("Consolas", 9))
            self.canvas.on_selection = self.show_selection
            controls = QWidget()
            form = QFormLayout(controls)
            self.seed_edit = QLineEdit(str(cfg.seed))
            self.x_edit, self.z_edit = QSpinBox(), QSpinBox()
            maximum_cell = (cfg.extent_cm // 2) // int(STANDARD_CELL_CM)
            for spin in (self.x_edit, self.z_edit): spin.setRange(-maximum_cell, maximum_cell)
            apply_seed = QPushButton("Apply seed")
            apply_seed.clicked.connect(self.apply_seed)
            jump = QPushButton("Jump to cell")
            jump.clicked.connect(lambda: self.canvas.jump(self.x_edit.value(), self.z_edit.value()))
            form.addRow("Seed (uint64)", self.seed_edit)
            form.addRow("Cell X", self.x_edit)
            form.addRow("Cell Z", self.z_edit)
            form.addRow(apply_seed)
            form.addRow(jump)
            self.legend = QLabel()
            self.legend.setTextFormat(Qt.RichText)
            form.addRow("Biomes", self.legend)
            form.addRow(QLabel(
                "Left click: inspect\nMiddle/right drag: pan\nWheel: zoom\n"
                "WASD/arrows: move\nHome: origin\n"
                "Green socket: connected\nRed socket: terminator"))
            form.addRow(self.details)
            splitter = QSplitter()
            splitter.addWidget(self.canvas)
            splitter.addWidget(controls)
            splitter.setSizes([950, 330])
            self.setCentralWidget(splitter)
            toolbar = QToolBar("Map")
            self.addToolBar(toolbar)
            open_action = QAction("Open scene.cfg", self)
            open_action.triggered.connect(self.open_scene)
            export_action = QAction("Export PNG", self)
            export_action.triggered.connect(self.export_png)
            origin_action = QAction("Go to origin", self)
            origin_action.triggered.connect(lambda: self.canvas.jump(0, 0))
            toolbar.addActions((open_action, export_action, origin_action))
            self.setStatusBar(QStatusBar())
            self.show_selection(world.inspect(0, 0))
            self.update_status()

        def update_status(self) -> None:
            self.legend.setText("<br>".join(
                f'<span style="color:{self.canvas.style_colour(style).name()}">■</span> {style}'
                for style in self.world.styles
            ))
            self.statusBar().showMessage(
                f"{self.config.path} | {len(self.world.tiles)} runtime tiles | "
                f"biome span {self.world.biome_span * 20} m | "
                f"world {self.config.extent_cm / 100000:.0f} km square"
            )

        def show_selection(self, info: dict[str, object]) -> None:
            self.x_edit.setValue(info["cell"][0])
            self.z_edit.setValue(info["cell"][1])
            self.details.setPlainText(json.dumps(info, indent=2))

        def apply_seed(self) -> None:
            try:
                seed = int(self.seed_edit.text(), 10)
                self.world = SeededWorld(self.world.manifest, seed, self.config.origin_block)
                self.config = WorldConfig(
                    self.config.path, seed, self.config.manifest_path,
                    self.config.origin_block, self.config.load_radius_cm,
                    self.config.vertical_radius_cm, self.config.extent_cm,
                    self.config.light_budget,
                )
                self.canvas.set_world(self.world, self.config)
                self.update_status()
            except Exception as error:
                QMessageBox.critical(self, "Invalid seed", str(error))

        def open_scene(self) -> None:
            selected, _ = QFileDialog.getOpenFileName(
                self, "Open Run3 world scene.cfg", str(self.config.path.parent), "scene.cfg (scene.cfg);;Config (*.cfg)"
            )
            if not selected: return
            try:
                cfg, world = create_world(Path(selected))
                self.config, self.world = cfg, world
                self.seed_edit.setText(str(cfg.seed))
                maximum = (cfg.extent_cm // 2) // int(STANDARD_CELL_CM)
                for spin in (self.x_edit, self.z_edit): spin.setRange(-maximum, maximum)
                self.canvas.set_world(world, cfg)
                self.update_status()
            except Exception as error:
                QMessageBox.critical(self, "Cannot open world", str(error))

        def export_png(self) -> None:
            selected, _ = QFileDialog.getSaveFileName(self, "Export visible map", "luniverse-map.png", "PNG image (*.png)")
            if not selected: return
            target = Path(selected)
            if target.suffix.lower() != ".png": target = target.with_suffix(".png")
            image = QImage(self.canvas.size(), QImage.Format_ARGB32)
            image.fill(Qt.transparent)
            painter = QPainter(image)
            self.canvas.render(painter)
            painter.end()
            if not image.save(str(target)):
                QMessageBox.critical(self, "Export failed", f"Could not write {target}")

    application = QApplication(sys.argv)
    application.setApplicationName("Run3 Luniverse Seed Map")
    window = Window(config, initial_world)
    window.show()
    if quit_after_ms > 0:
        QTimer.singleShot(quit_after_ms, application.quit)
    return application.exec_()


def arguments(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scene", type=Path, default=default_scene(), help="world scene.cfg")
    parser.add_argument("--content-root", type=Path, help="root used to resolve run3/... manifest paths")
    parser.add_argument("--seed", type=int, help="temporarily override WorldSeed")
    parser.add_argument("--dump", nargs=3, type=int, metavar=("X", "Z", "RADIUS"),
                        help="print a JSON cell region instead of opening Qt")
    parser.add_argument("--self-test", action="store_true",
                        help="verify generator parity, occupancy, and all manifest meshes")
    parser.add_argument("--gui-smoke", action="store_true", help=argparse.SUPPRESS)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    options = arguments(argv)
    try:
        config, world = create_world(options.scene, options.content_root)
        if options.seed is not None:
            world = SeededWorld(world.manifest, options.seed, config.origin_block)
            config = WorldConfig(config.path, options.seed, config.manifest_path,
                                 config.origin_block, config.load_radius_cm,
                                 config.vertical_radius_cm, config.extent_cm,
                                 config.light_budget)
        if options.self_test:
            verify_runtime_vectors(world.manifest)
            print("Luniverse generator parity vectors: OK")
            return 0
        if options.dump:
            x, z, radius = options.dump
            if radius < 0 or radius > 100:
                raise ValueError("dump radius must be between 0 and 100")
            dump_region(world, x, z, radius)
            return 0
        return run_gui(config, world, 250 if options.gui_smoke else 0)
    except Exception as error:
        print(f"luniverse_map_viewer: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
