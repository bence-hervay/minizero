#pragma once

#include <cassert>
#include <string>

namespace minizero::utils {

// A generic symmetry handle. The meaning of the id is environment-specific.
class Symmetry {
public:
    constexpr explicit Symmetry(int id = 0)
        : id_(id)
    {
    }

    constexpr int getID() const { return id_; }
    constexpr bool operator==(const Symmetry& rhs) const { return id_ == rhs.id_; }
    constexpr bool operator!=(const Symmetry& rhs) const { return !(*this == rhs); }

private:
    int id_;
};

enum class Rotation {
    kRotationNone,
    kRotation90,
    kRotation180,
    kRotation270,
    kHorizontalRotation,
    kHorizontalRotation90,
    kHorizontalRotation180,
    kHorizontalRotation270,
    kRotateSize
};

inline constexpr int kRotationSymmetryCount = static_cast<int>(Rotation::kRotateSize);

inline constexpr Rotation reversed_rotation[kRotationSymmetryCount] = {
    Rotation::kRotationNone,
    Rotation::kRotation270,
    Rotation::kRotation180,
    Rotation::kRotation90,
    Rotation::kHorizontalRotation,
    Rotation::kHorizontalRotation90,
    Rotation::kHorizontalRotation180,
    Rotation::kHorizontalRotation270};

inline const std::string rotation_string[kRotationSymmetryCount] = {
    "Rotation_None",
    "Rotation_90_Degree",
    "Rotation_180_Degree",
    "Rotation_270_Degree",
    "Horizontal_Rotation",
    "Horizontal_Rotation_90_Degree",
    "Horizontal_Rotation_180_Degree",
    "Horizontal_Rotation_270_Degree"};

inline constexpr Symmetry rotationToSymmetry(Rotation rotation) { return Symmetry(static_cast<int>(rotation)); }

inline Rotation symmetryToRotation(Symmetry symmetry)
{
    assert(symmetry.getID() >= 0 && symmetry.getID() < kRotationSymmetryCount);
    return static_cast<Rotation>(symmetry.getID());
}

inline constexpr int getRotationSymmetryCount() { return kRotationSymmetryCount; }
inline constexpr Symmetry getIdentitySymmetry() { return rotationToSymmetry(Rotation::kRotationNone); }
inline std::string getRotationString(Rotation rotation) { return rotation_string[static_cast<int>(rotation)]; }
inline std::string getRotationSymmetryString(Symmetry symmetry) { return getRotationString(symmetryToRotation(symmetry)); }

inline Rotation getRotationFromString(const std::string rotation_str)
{
    for (int i = 0; i < kRotationSymmetryCount; ++i) {
        if (rotation_str == rotation_string[i]) { return static_cast<Rotation>(i); }
    }
    return Rotation::kRotateSize;
}

inline Symmetry getSymmetryFromRotationString(const std::string& rotation_str)
{
    return rotationToSymmetry(getRotationFromString(rotation_str));
}

inline Symmetry getReversedSymmetry(Symmetry symmetry)
{
    return rotationToSymmetry(reversed_rotation[static_cast<int>(symmetryToRotation(symmetry))]);
}

inline int getPositionByRotating(Rotation rotation, int original_pos, int board_size)
{
    assert(original_pos >= 0 && original_pos <= board_size * board_size);
    if (original_pos == board_size * board_size) { return original_pos; }

    const float center = (board_size - 1) / 2.0;
    float x = original_pos % board_size - center;
    float y = original_pos / board_size - center;
    float rotation_x = x, rotation_y = y;
    switch (rotation) {
        case Rotation::kRotationNone:
            rotation_x = x, rotation_y = y;
            break;
        case Rotation::kRotation90:
            rotation_x = y, rotation_y = -x;
            break;
        case Rotation::kRotation180:
            rotation_x = -x, rotation_y = -y;
            break;
        case Rotation::kRotation270:
            rotation_x = -y, rotation_y = x;
            break;
        case Rotation::kHorizontalRotation:
            rotation_x = x, rotation_y = -y;
            break;
        case Rotation::kHorizontalRotation90:
            rotation_x = -y, rotation_y = -x;
            break;
        case Rotation::kHorizontalRotation180:
            rotation_x = -x, rotation_y = y;
            break;
        case Rotation::kHorizontalRotation270:
            rotation_x = y, rotation_y = x;
            break;
        default:
            assert(false);
            break;
    }

    int new_pos = (rotation_y + center) * board_size + (rotation_x + center);
    assert(new_pos >= 0 && new_pos < board_size * board_size);
    return new_pos;
}

inline int getPositionBySymmetry(Symmetry symmetry, int original_pos, int board_size)
{
    return getPositionByRotating(symmetryToRotation(symmetry), original_pos, board_size);
}

} // namespace minizero::utils
