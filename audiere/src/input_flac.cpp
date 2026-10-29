#include "input_flac.h"
#include "types.h"
#include <algorithm>
#include <limits>

namespace audiere {
FLACInputStream::FLACInputStream() = default;
FLACInputStream::~FLACInputStream() {
  if (m_decoder) {
    FLAC__stream_decoder_finish(m_decoder);
    FLAC__stream_decoder_delete(m_decoder);
  }
}

bool FLACInputStream::initialize(FilePtr file) {
  m_file = file;
  m_decoder = FLAC__stream_decoder_new();
  if (!m_decoder) return false;
  const auto status = FLAC__stream_decoder_init_stream(
      m_decoder, readCallback, seekCallback, tellCallback, lengthCallback,
      eofCallback, writeCallback, metadataCallback, errorCallback, this);
  if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK ||
      !FLAC__stream_decoder_process_until_end_of_metadata(m_decoder)) {
    FLAC__stream_decoder_delete(m_decoder);
    m_decoder = nullptr;
    m_file = nullptr;
    return false;
  }
  const unsigned bits = m_bits_per_sample;
  if (m_channel_count < 1 || m_channel_count > 2 || m_sample_rate <= 0 ||
      bits == 0 || bits > 32) return false;
  m_bits_per_sample = bits;
  m_sample_format = bits <= 8 ? SF_U8 : SF_S16;
  return true;
}

bool FLACInputStream::initialize(const char *filename) {
  if (!filename) return false;
  m_decoder = FLAC__stream_decoder_new();
  if (!m_decoder) return false;
  const auto status = FLAC__stream_decoder_init_file(
      m_decoder, filename, writeCallback, metadataCallback, errorCallback,
      this);
  const bool metadata = status == FLAC__STREAM_DECODER_INIT_STATUS_OK &&
      FLAC__stream_decoder_process_until_end_of_metadata(m_decoder);
  if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK || !metadata) {
    FLAC__stream_decoder_delete(m_decoder);
    m_decoder = nullptr;
    return false;
  }
  const unsigned bits = m_bits_per_sample;
  if (m_channel_count < 1 || m_channel_count > 2 || m_sample_rate <= 0 ||
      bits == 0 || bits > 32) return false;
  m_bits_per_sample = bits;
  m_sample_format = bits <= 8 ? SF_U8 : SF_S16;
  return true;
}

void FLACInputStream::getFormat(int &channels, int &rate,
                                SampleFormat &format) {
  channels = m_channel_count;
  rate = m_sample_rate;
  format = m_sample_format;
}

int FLACInputStream::doRead(const int frame_count, void *samples) {
  const int frame_size = m_channel_count * GetSampleSize(m_sample_format);
  auto *out = static_cast<u8 *>(samples);
  int frames_read{};
  while (frames_read < frame_count) {
    if (m_buffer.getSize() < frame_size) {
      if (!FLAC__stream_decoder_process_single(m_decoder) ||
          FLAC__stream_decoder_get_state(m_decoder) ==
              FLAC__STREAM_DECODER_END_OF_STREAM) break;
      if (m_buffer.getSize() < frame_size) break;
    }
    const int count = std::min(frame_count - frames_read,
                               m_buffer.getSize() / frame_size);
    m_buffer.read(out, count * frame_size);
    out += count * frame_size;
    frames_read += count;
  }
  return frames_read;
}

void FLACInputStream::reset() {
  if (m_decoder) FLAC__stream_decoder_seek_absolute(m_decoder, 0);
  m_position = 0;
  m_buffer.clear();
}
bool FLACInputStream::isSeekable() { return m_length > 0; }
int FLACInputStream::getLength() { return m_length; }
void FLACInputStream::setPosition(const int position) {
  if (position >= 0 && m_decoder && FLAC__stream_decoder_seek_absolute(
          m_decoder, static_cast<FLAC__uint64>(position))) {
    m_position = position;
    m_buffer.clear();
  }
}
int FLACInputStream::getPosition() {
  const int frame_size = m_channel_count * GetSampleSize(m_sample_format);
  return frame_size > 0 ? m_position - m_buffer.getSize() / frame_size
                        : m_position;
}

FLAC__StreamDecoderWriteStatus FLACInputStream::write(
    const FLAC__Frame *frame, const FLAC__int32 *const buffer[]) {
  const int channels = static_cast<int>(frame->header.channels);
  const int samples = static_cast<int>(frame->header.blocksize);
  const int output_bytes = GetSampleSize(m_sample_format);
  const int total = channels * samples * output_bytes;
  m_multiplexer.ensureSize(total);
  if (m_sample_format == SF_U8) {
    auto *out = static_cast<u8 *>(m_multiplexer.get());
    const unsigned shift = m_bits_per_sample > 8 ? m_bits_per_sample - 8 : 0;
    for (int s = 0; s < samples; ++s)
      for (int c = 0; c < channels; ++c)
        *out++ = static_cast<u8>((buffer[c][s] >> shift) + 128);
  } else {
    auto *out = static_cast<s16 *>(m_multiplexer.get());
    const unsigned right_shift = m_bits_per_sample > 16
        ? m_bits_per_sample - 16 : 0;
    const unsigned left_shift = m_bits_per_sample < 16
        ? 16 - m_bits_per_sample : 0;
    for (int s = 0; s < samples; ++s)
      for (int c = 0; c < channels; ++c)
        *out++ = static_cast<s16>(right_shift
            ? buffer[c][s] >> right_shift
            : buffer[c][s] * static_cast<FLAC__int32>(1U << left_shift));
  }
  m_buffer.write(m_multiplexer.get(), total);
  m_position += samples;
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus FLACInputStream::readCallback(
    const FLAC__StreamDecoder *, FLAC__byte buffer[], size_t *bytes,
    void *data) {
  auto *self = static_cast<FLACInputStream *>(data);
  const int requested = static_cast<int>(std::min<size_t>(
      *bytes, static_cast<size_t>(std::numeric_limits<int>::max())));
  const int read = self->m_file->read(buffer, requested);
  *bytes = static_cast<size_t>(std::max(0, read));
  return read > 0 ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE
                  : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}
FLAC__StreamDecoderSeekStatus FLACInputStream::seekCallback(
    const FLAC__StreamDecoder *, const FLAC__uint64 offset, void *data) {
  auto *self = static_cast<FLACInputStream *>(data);
  if (offset > static_cast<FLAC__uint64>(std::numeric_limits<int>::max()))
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  return self->m_file->seek(static_cast<int>(offset), File::BEGIN)
      ? FLAC__STREAM_DECODER_SEEK_STATUS_OK
      : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}
FLAC__StreamDecoderTellStatus FLACInputStream::tellCallback(
    const FLAC__StreamDecoder *, FLAC__uint64 *offset, void *data) {
  *offset = static_cast<FLAC__uint64>(
      static_cast<FLACInputStream *>(data)->m_file->tell());
  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}
FLAC__StreamDecoderLengthStatus FLACInputStream::lengthCallback(
    const FLAC__StreamDecoder *, FLAC__uint64 *length, void *data) {
  auto *self = static_cast<FLACInputStream *>(data);
  const int current = self->m_file->tell();
  self->m_file->seek(0, File::END);
  *length = static_cast<FLAC__uint64>(self->m_file->tell());
  self->m_file->seek(current, File::BEGIN);
  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}
FLAC__bool FLACInputStream::eofCallback(const FLAC__StreamDecoder *,
                                        void *data) {
  auto *self = static_cast<FLACInputStream *>(data);
  const int current = self->m_file->tell();
  self->m_file->seek(0, File::END);
  const int end = self->m_file->tell();
  self->m_file->seek(current, File::BEGIN);
  return current >= end;
}
FLAC__StreamDecoderWriteStatus FLACInputStream::writeCallback(
    const FLAC__StreamDecoder *, const FLAC__Frame *frame,
    const FLAC__int32 *const buffer[], void *data) {
  return static_cast<FLACInputStream *>(data)->write(frame, buffer);
}
void FLACInputStream::metadataCallback(const FLAC__StreamDecoder *,
    const FLAC__StreamMetadata *metadata, void *data) {
  if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO) return;
  auto *self = static_cast<FLACInputStream *>(data);
  self->m_channel_count = static_cast<int>(
      metadata->data.stream_info.channels);
  self->m_sample_rate = static_cast<int>(
      metadata->data.stream_info.sample_rate);
  self->m_bits_per_sample = metadata->data.stream_info.bits_per_sample;
  const auto samples = metadata->data.stream_info.total_samples;
  self->m_length =
      samples > static_cast<FLAC__uint64>(std::numeric_limits<int>::max())
          ? 0 : static_cast<int>(samples);
}
void FLACInputStream::errorCallback(const FLAC__StreamDecoder *,
    FLAC__StreamDecoderErrorStatus, void *) {}
} // namespace audiere
