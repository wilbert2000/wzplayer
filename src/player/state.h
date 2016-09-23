#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

namespace Player {

enum TState {
    STATE_STOPPED = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_STOPPING,
    STATE_RESTARTING,
    STATE_LOADING
};

} // namespace Player

#endif // PLAYER_STATE_H
