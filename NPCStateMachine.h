#pragma once
#include <string>

// Simple state machine for NPCs
enum class NPCState {
    Idle,
    Moving,
    Attacking,
    Dead
};

class NPCStateMachine {
public:
    NPCStateMachine() : mState(NPCState::Idle) {}
    void setState(NPCState state) { mState = state; }
    NPCState getState() const { return mState; }
    bool is(NPCState state) const { return mState == state; }
private:
    NPCState mState;
};
