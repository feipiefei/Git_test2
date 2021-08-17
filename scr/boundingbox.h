#pragma once
#include <cfloat>
#include <algorithm>
#include <glm/glm.hpp>

class BoundingBox {
public:
  BoundingBox()noexcept
    : x_min_(FLT_MAX),
      x_max_(-FLT_MAX),
      y_min_(FLT_MAX),
      y_max_(-FLT_MAX),
      z_min_(FLT_MAX),
      z_max_(-FLT_MAX) {
  }

  void Clear() {
    x_min_ = FLT_MAX;
    y_min_ = FLT_MAX;
    z_min_ = FLT_MAX;
    x_max_ = -FLT_MAX;
    y_max_ = -FLT_MAX;
    z_max_ = -FLT_MAX;
  }

  void AddPoint3(glm::fvec3 point3) {
    x_min_ = std::min(x_min_, point3[0]);
    y_min_ = std::min(y_min_, point3[1]);
    z_min_ = std::min(z_min_, point3[2]);
    x_max_ = std::max(x_max_, point3[0]);
    y_max_ = std::max(y_max_, point3[1]);
    z_max_ = std::max(z_max_, point3[2]);
  }

  void AddBoundingBox(const BoundingBox &another_box)
  {
    AddPoint3({another_box.x_min(),another_box.y_min(),another_box.z_min()});
    AddPoint3({another_box.x_max(),another_box.y_max(),another_box.z_max()});
  }

  float x_min() const {
    return x_min_;
  }

  float x_max() const {
    return x_max_;
  }

  float y_min() const {
    return y_min_;
  }

  float y_max() const {
    return y_max_;
  }

  float z_min() const {
    return z_min_;
  }

  float z_max() const {
    return z_max_;
  }

  float length() const {
    return x_max_ - x_min_;
  }

  float width() const {
    return y_max_ - y_min_;
  }

  float height() const {
    return z_max_ - z_min_;
  }

  glm::fvec3 center() const {
    return glm::fvec3(
      (x_min_ + x_max_) / 2,
      (y_min_ + y_max_) / 2,
      (z_min_ + z_max_) / 2
    );
  }

  float radius() const
  {
    return std::sqrt(length()*length()+width()*width()+height()*height())/2;
  }

private:
  float x_min_, x_max_, y_min_, y_max_, z_min_, z_max_;
};

