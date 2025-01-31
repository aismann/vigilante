// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#ifndef VIGILANTE_INTERACT_WITH_TARGET_OBJECTIVE_H_
#define VIGILANTE_INTERACT_WITH_TARGET_OBJECTIVE_H_

#include <string>

#include "Quest.h"

namespace vigilante {

class InteractWithTargetObjective : public Quest::Objective {
 public:
  InteractWithTargetObjective(const std::string& desc, const std::string &targetProfileJsonFileName)
      : Quest::Objective{Quest::Objective::Type::INTERACT_WITH, desc},
        _targetProfileJsonFileName{targetProfileJsonFileName} {}
  virtual ~InteractWithTargetObjective() = default;

  virtual bool isCompleted() const override { return _is_completed; }

  inline const std::string &getTargetProfileJsonFileName() const { return _targetProfileJsonFileName; }
  inline void setCompleted(const bool is_completed) { return _is_completed = is_completed; }

 private:
  const std::string _targetProfileJsonFileName;
  bool _is_completed{false};
};

}  // namespace vigilante

#endif  // VIGILANTE_INTERACT_WITH_TARGET_OBJECTIVE_H_
