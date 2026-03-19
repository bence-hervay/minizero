#pragma once

#include "base_env.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace minizero::env::hextictactoe {

const std::string kHexTicTacToeName = "hextictactoe";
const int kHexTicTacToeNumPlayer = 2;
const int kHexTicTacToeNumWinConnectStone = 6;

struct HexTicTacToeCoord {
    int x;
    int y;

    inline HexTicTacToeCoord operator+(const HexTicTacToeCoord& rhs) const { return {x + rhs.x, y + rhs.y}; }
    inline HexTicTacToeCoord operator-(const HexTicTacToeCoord& rhs) const { return {x - rhs.x, y - rhs.y}; }
    inline HexTicTacToeCoord operator*(int scalar) const { return {x * scalar, y * scalar}; }
};

class HexTicTacToeAction : public BaseBoardAction<kHexTicTacToeNumPlayer> {
public:
    HexTicTacToeAction() : BaseBoardAction<kHexTicTacToeNumPlayer>() {}
    HexTicTacToeAction(int action_id, Player player) : BaseBoardAction<kHexTicTacToeNumPlayer>(action_id, player) {}
    HexTicTacToeAction(const std::vector<std::string>& action_string_args, int board_size = minizero::config::env_board_size)
        : BaseBoardAction<kHexTicTacToeNumPlayer>(action_string_args, board_size) {}

    inline Player nextPlayer() const override { throw std::runtime_error{"MuZero does not support this game"}; }
    inline Player nextPlayer(int move_id) const { return (move_id % 2 == 1 ? getPlayer() : BaseBoardAction::nextPlayer()); }
};

class HexTicTacToeEnv : public BaseBoardEnv<HexTicTacToeAction> {
public:
    HexTicTacToeEnv() { reset(); }

    void reset() override;
    bool act(const HexTicTacToeAction& action) override;
    bool act(const std::vector<std::string>& action_string_args) override;
    std::vector<HexTicTacToeAction> getLegalActions() const override;
    bool isLegalAction(const HexTicTacToeAction& action) const override;
    bool isTerminal() const override;
    float getReward() const override { return 0.0f; }
    float getEvalScore(bool is_resign = false) const override;
    std::vector<float> getFeatures(utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    std::vector<float> getActionFeatures(const HexTicTacToeAction& action, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    inline int getNumInputChannels() const override { return 5; }
    inline int getPolicySize() const override { return getBoardSize() * getBoardSize(); }
    std::string toString() const override;
    inline std::string name() const override { return kHexTicTacToeName + "_" + std::to_string(getBoardSize()); }
    inline int getNumPlayer() const override { return kHexTicTacToeNumPlayer; }
    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; }
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; }

private:
    Player updateWinner(const HexTicTacToeAction& action) const;
    bool hasWinningLine(const HexTicTacToeAction& action, const HexTicTacToeCoord& direction) const;
    inline HexTicTacToeCoord actionIDToCoord(int action_id) const { return {action_id % getBoardSize(), action_id / getBoardSize()}; }
    inline int coordToActionID(const HexTicTacToeCoord& coord) const { return wrapCoordinate(coord.y) * getBoardSize() + wrapCoordinate(coord.x); }
    inline int wrapCoordinate(int value) const { return (value % getBoardSize() + getBoardSize()) % getBoardSize(); }
    inline int getCenterPosition() const { return (getBoardSize() / 2) * getBoardSize() + getBoardSize() / 2; }
    Player getPlayerAtCoord(const HexTicTacToeCoord& coord) const;
    std::string getCoordinateString() const;

    Player winner_;
    std::vector<Player> board_;
};

class HexTicTacToeEnvLoader : public BaseBoardEnvLoader<HexTicTacToeAction, HexTicTacToeEnv> {
public:
    std::vector<float> getActionFeatures(const int pos, utils::Rotation rotation = utils::Rotation::kRotationNone) const override;
    inline std::vector<float> getValue(const int pos) const { return {getReturn()}; }
    inline std::string name() const override { return kHexTicTacToeName + "_" + std::to_string(getBoardSize()); }
    inline int getPolicySize() const override { return getBoardSize() * getBoardSize(); }
    inline int getRotatePosition(int position, utils::Rotation rotation) const override { return position; }
    inline int getRotateAction(int action_id, utils::Rotation rotation) const override { return action_id; }
};

} // namespace minizero::env::hextictactoe
