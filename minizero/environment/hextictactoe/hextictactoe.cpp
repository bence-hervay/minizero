#include "hextictactoe.h"
#include "random.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace minizero::env::hextictactoe {

using namespace minizero::utils;

namespace {

constexpr int kHexTicTacToeRotationSymmetryCount = 6;
constexpr int kHexTicTacToeSymmetryCount = 12;
constexpr int kHexTicTacToeReflectionOffset = 6;
constexpr int kHexTicTacToeInverseSymmetry[kHexTicTacToeSymmetryCount] = {
    0, 5, 4, 3, 2, 1,
    6, 7, 8, 9, 10, 11};
const std::string kHexTicTacToeSymmetryStrings[kHexTicTacToeSymmetryCount] = {
    "Hex_Rotation_0_Degree",
    "Hex_Rotation_60_Degree",
    "Hex_Rotation_120_Degree",
    "Hex_Rotation_180_Degree",
    "Hex_Rotation_240_Degree",
    "Hex_Rotation_300_Degree",
    "Hex_Reflection_0_Degree",
    "Hex_Reflection_60_Degree",
    "Hex_Reflection_120_Degree",
    "Hex_Reflection_180_Degree",
    "Hex_Reflection_240_Degree",
    "Hex_Reflection_300_Degree"};

HexTicTacToeCoord getCenterCoord(int board_size)
{
    return {board_size / 2, board_size / 2};
}

int wrapHexCoordinate(int value, int board_size)
{
    return (value % board_size + board_size) % board_size;
}

HexTicTacToeCoord wrapHexCoord(const HexTicTacToeCoord& coord, int board_size)
{
    return {wrapHexCoordinate(coord.x, board_size), wrapHexCoordinate(coord.y, board_size)};
}

HexTicTacToeCoord rotateRelativeCoord(const HexTicTacToeCoord& coord, int rotation_id)
{
    assert(rotation_id >= 0 && rotation_id < kHexTicTacToeRotationSymmetryCount);
    switch (rotation_id) {
        case 0: return coord;
        case 1: return {-coord.y, coord.x + coord.y};
        case 2: return {-coord.x - coord.y, coord.x};
        case 3: return {-coord.x, -coord.y};
        case 4: return {coord.y, -coord.x - coord.y};
        case 5: return {coord.x + coord.y, -coord.x};
        default: assert(false); return coord;
    }
}

HexTicTacToeCoord reflectRelativeCoord(const HexTicTacToeCoord& coord)
{
    return {coord.y, coord.x};
}

HexTicTacToeCoord transformRelativeCoord(const HexTicTacToeCoord& coord, int symmetry_id)
{
    assert(symmetry_id >= 0 && symmetry_id < kHexTicTacToeSymmetryCount);
    HexTicTacToeCoord transformed = rotateRelativeCoord(coord, symmetry_id % kHexTicTacToeRotationSymmetryCount);
    if (symmetry_id >= kHexTicTacToeReflectionOffset) { transformed = reflectRelativeCoord(transformed); }
    return transformed;
}

HexTicTacToeCoord transformHexCoord(const HexTicTacToeCoord& coord, int symmetry_id, int board_size)
{
    const HexTicTacToeCoord center = getCenterCoord(board_size);
    const HexTicTacToeCoord relative = coord - center;
    return wrapHexCoord(center + transformRelativeCoord(relative, symmetry_id), board_size);
}

} // namespace

void HexTicTacToeEnv::reset()
{
    winner_ = Player::kPlayerNone;
    turn_ = Player::kPlayer2;
    actions_.clear();
    board_.assign(getPolicySize(), Player::kPlayerNone);
    board_[getCenterPosition()] = Player::kPlayer1;
}

bool HexTicTacToeEnv::act(const HexTicTacToeAction& action)
{
    if (!isLegalAction(action)) { return false; }
    actions_.push_back(action);
    board_[action.getActionID()] = action.getPlayer();
    turn_ = action.nextPlayer(actions_.size());
    winner_ = updateWinner(action);
    return true;
}

bool HexTicTacToeEnv::act(const std::vector<std::string>& action_string_args)
{
    return act(HexTicTacToeAction(action_string_args));
}

std::vector<HexTicTacToeAction> HexTicTacToeEnv::getLegalActions() const
{
    std::vector<HexTicTacToeAction> actions;
    for (int action_id = 0; action_id < getPolicySize(); ++action_id) {
        HexTicTacToeAction action(action_id, turn_);
        if (!isLegalAction(action)) { continue; }
        actions.push_back(action);
    }
    return actions;
}

bool HexTicTacToeEnv::isLegalAction(const HexTicTacToeAction& action) const
{
    assert(action.getActionID() >= 0 && action.getActionID() < getPolicySize());
    assert(action.getPlayer() == Player::kPlayer1 || action.getPlayer() == Player::kPlayer2);

    return (action.getPlayer() == turn_ &&
            action.getActionID() >= 0 &&
            action.getActionID() < getPolicySize() &&
            board_[action.getActionID()] == Player::kPlayerNone);
}

bool HexTicTacToeEnv::isTerminal() const
{
    return (winner_ != Player::kPlayerNone || std::find(board_.begin(), board_.end(), Player::kPlayerNone) == board_.end());
}

float HexTicTacToeEnv::getEvalScore(bool is_resign /* = false */) const
{
    Player result = (is_resign ? getNextPlayer(turn_, kHexTicTacToeNumPlayer) : winner_);
    switch (result) {
        case Player::kPlayer1: return 1.0f;
        case Player::kPlayer2: return -1.0f;
        default: return 0.0f;
    }
}

std::vector<float> HexTicTacToeEnv::getFeatures(utils::Rotation rotation /*= utils::Rotation::kRotationNone*/) const
{
    /* 5 channels:
        0~1. own/opponent position
        2. Player 1 turn
        3. Player 2 turn
        4. one move left in the current two-step turn
    */
    std::vector<float> features;
    const bool one_move_left = (actions_.size() % 2 == 1);
    for (int channel = 0; channel < getNumInputChannels(); ++channel) {
        for (int pos = 0; pos < getPolicySize(); ++pos) {
            int rotation_pos = getRotatePosition(pos, utils::reversed_rotation[static_cast<int>(rotation)]);
            if (channel == 0) {
                features.push_back(board_[rotation_pos] == turn_ ? 1.0f : 0.0f);
            } else if (channel == 1) {
                features.push_back(board_[rotation_pos] == getNextPlayer(turn_, kHexTicTacToeNumPlayer) ? 1.0f : 0.0f);
            } else if (channel == 2) {
                features.push_back(turn_ == Player::kPlayer1 ? 1.0f : 0.0f);
            } else if (channel == 3) {
                features.push_back(turn_ == Player::kPlayer2 ? 1.0f : 0.0f);
            } else {
                features.push_back(one_move_left ? 1.0f : 0.0f);
            }
        }
    }
    return features;
}

int HexTicTacToeEnv::getNumSymmetries() const
{
    return kHexTicTacToeSymmetryCount;
}

utils::Symmetry HexTicTacToeEnv::getIdentitySymmetry() const
{
    return utils::Symmetry(0);
}

utils::Symmetry HexTicTacToeEnv::getSymmetry(int symmetry_id) const
{
    assert(symmetry_id >= 0 && symmetry_id < getNumSymmetries());
    return utils::Symmetry(symmetry_id);
}

utils::Symmetry HexTicTacToeEnv::getInverseSymmetry(utils::Symmetry symmetry) const
{
    assert(symmetry.getID() >= 0 && symmetry.getID() < getNumSymmetries());
    return utils::Symmetry(kHexTicTacToeInverseSymmetry[symmetry.getID()]);
}

std::string HexTicTacToeEnv::getSymmetryString(utils::Symmetry symmetry) const
{
    assert(symmetry.getID() >= 0 && symmetry.getID() < getNumSymmetries());
    return kHexTicTacToeSymmetryStrings[symmetry.getID()];
}

std::vector<float> HexTicTacToeEnv::getFeaturesBySymmetry(utils::Symmetry symmetry /*= utils::Symmetry()*/) const
{
    std::vector<float> features;
    const utils::Symmetry inverse_symmetry = getInverseSymmetry(symmetry);
    const bool one_move_left = (actions_.size() % 2 == 1);
    for (int channel = 0; channel < getNumInputChannels(); ++channel) {
        for (int pos = 0; pos < getPolicySize(); ++pos) {
            const int symmetry_pos = getSymmetryPosition(pos, inverse_symmetry);
            if (channel == 0) {
                features.push_back(board_[symmetry_pos] == turn_ ? 1.0f : 0.0f);
            } else if (channel == 1) {
                features.push_back(board_[symmetry_pos] == getNextPlayer(turn_, kHexTicTacToeNumPlayer) ? 1.0f : 0.0f);
            } else if (channel == 2) {
                features.push_back(turn_ == Player::kPlayer1 ? 1.0f : 0.0f);
            } else if (channel == 3) {
                features.push_back(turn_ == Player::kPlayer2 ? 1.0f : 0.0f);
            } else {
                features.push_back(one_move_left ? 1.0f : 0.0f);
            }
        }
    }
    return features;
}

std::vector<float> HexTicTacToeEnv::getActionFeaturesBySymmetry(const HexTicTacToeAction& action, utils::Symmetry symmetry /* = utils::Symmetry() */) const
{
    std::vector<float> action_features(getPolicySize(), 0.0f);
    action_features[getSymmetryAction(action.getActionID(), symmetry)] = 1.0f;
    return action_features;
}

int HexTicTacToeEnv::getSymmetryPosition(int position, utils::Symmetry symmetry) const
{
    assert(position >= 0 && position < getPolicySize());
    return coordToActionID(transformHexCoord(actionIDToCoord(position), symmetry.getID(), getBoardSize()));
}

int HexTicTacToeEnv::getSymmetryAction(int action_id, utils::Symmetry symmetry) const
{
    return getSymmetryPosition(action_id, symmetry);
}

std::vector<float> HexTicTacToeEnv::getActionFeatures(const HexTicTacToeAction& action, utils::Rotation rotation /* = utils::Rotation::kRotationNone */) const
{
    std::vector<float> action_features(getPolicySize(), 0.0f);
    action_features[getRotateAction(action.getActionID(), rotation)] = 1.0f;
    return action_features;
}

std::string HexTicTacToeEnv::toString() const
{
    std::ostringstream oss;
    oss << "   " << getCoordinateString() << std::endl;
    for (int row = getBoardSize() - 1; row >= 0; --row) {
        oss << std::string(getBoardSize() - 1 - row, ' ');
        oss << std::setw(2) << row + 1 << " ";
        for (int col = 0; col < getBoardSize(); ++col) {
            Player player = board_[row * getBoardSize() + col];
            if (player == Player::kPlayer1) {
                oss << "O ";
            } else if (player == Player::kPlayer2) {
                oss << "X ";
            } else {
                oss << ". ";
            }
        }
        oss << std::setw(2) << row + 1 << std::endl;
    }
    oss << std::string(getBoardSize() - 1, ' ') << "   " << getCoordinateString() << std::endl;
    return oss.str();
}

Player HexTicTacToeEnv::updateWinner(const HexTicTacToeAction& action) const
{
    if (hasWinningLine(action, {1, 0})) { return action.getPlayer(); }
    if (hasWinningLine(action, {0, 1})) { return action.getPlayer(); }
    if (hasWinningLine(action, {1, -1})) { return action.getPlayer(); }
    return Player::kPlayerNone;
}

bool HexTicTacToeEnv::hasWinningLine(const HexTicTacToeAction& action, const HexTicTacToeCoord& direction) const
{
    if (getBoardSize() < kHexTicTacToeNumWinConnectStone) { return false; }

    HexTicTacToeCoord origin = actionIDToCoord(action.getActionID());
    int line_length = 1;
    for (int step = 1; step < getBoardSize() && line_length < kHexTicTacToeNumWinConnectStone; ++step) {
        if (getPlayerAtCoord(origin + direction * step) != action.getPlayer()) { break; }
        ++line_length;
    }
    for (int step = 1; step < getBoardSize() && line_length < kHexTicTacToeNumWinConnectStone; ++step) {
        if (getPlayerAtCoord(origin - direction * step) != action.getPlayer()) { break; }
        ++line_length;
    }
    return line_length >= kHexTicTacToeNumWinConnectStone;
}

Player HexTicTacToeEnv::getPlayerAtCoord(const HexTicTacToeCoord& coord) const
{
    return board_[coordToActionID(coord)];
}

std::string HexTicTacToeEnv::getCoordinateString() const
{
    std::ostringstream oss;
    for (int i = 0; i < getBoardSize(); ++i) {
        char c = 'A' + i + (i >= 8);
        oss << c << " ";
    }
    return oss.str();
}

std::vector<float> HexTicTacToeEnvLoader::getActionFeatures(const int pos, utils::Rotation rotation /* = utils::Rotation::kRotationNone */) const
{
    const HexTicTacToeAction& action = action_pairs_[pos].first;
    std::vector<float> action_features(getPolicySize(), 0.0f);
    int action_id = ((pos < static_cast<int>(action_pairs_.size())) ? getRotateAction(action.getActionID(), rotation) : utils::Random::randInt() % action_features.size());
    action_features[action_id] = 1.0f;
    return action_features;
}

std::vector<float> HexTicTacToeEnvLoader::getActionFeaturesBySymmetry(const int pos, utils::Symmetry symmetry /* = utils::Symmetry() */) const
{
    const HexTicTacToeAction& action = action_pairs_[pos].first;
    std::vector<float> action_features(getPolicySize(), 0.0f);
    const int action_id = ((pos < static_cast<int>(action_pairs_.size())) ? getSymmetryAction(action.getActionID(), symmetry) : utils::Random::randInt() % action_features.size());
    action_features[action_id] = 1.0f;
    return action_features;
}

} // namespace minizero::env::hextictactoe
