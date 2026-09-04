#include "StateMachine.h"

void StateMachine::registerScreen(ScreenId id, ScreenFactory factory) {
    factories_[id] = std::move(factory);
}

void StateMachine::applySwitch(ScreenId id) {
    auto it = factories_.find(id);
    if (it == factories_.end()) {
        return; // nicht registriert - Zustand bleibt bewusst unveraendert statt abzustuerzen
    }

    if (current_) {
        current_->onExit();
    }

    current_ = it->second();
    currentId_ = id;

    if (current_) {
        current_->onEnter();
    }
}

void StateMachine::switchTo(ScreenId id) {
    applySwitch(id);
}

void StateMachine::requestSwitch(ScreenId id) {
    hasPendingSwitch_ = true;
    pendingId_ = id;
}

void StateMachine::update(uint32_t deltaMs) {
    if (hasPendingSwitch_) {
        hasPendingSwitch_ = false;
        applySwitch(pendingId_);
    }
    if (current_) {
        current_->update(deltaMs);
    }
}

void StateMachine::draw() {
    if (current_) {
        current_->draw();
    }
}
