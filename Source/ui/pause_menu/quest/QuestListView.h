// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#ifndef VIGILANTE_QUEST_LIST_VIEW_H_
#define VIGILANTE_QUEST_LIST_VIEW_H_

#include <string>

#include "quest/Quest.h"
#include "ui/ListView.h"

namespace vigilante {

class PauseMenu;

class QuestListView : public ListView<Quest*> {
 public:
  explicit QuestListView(PauseMenu* pauseMenu);
  virtual ~QuestListView() = default;

  virtual void confirm() override;  // ListView<Quest*>
  virtual void selectUp() override;  // ListView<Quest*>
  virtual void selectDown() override;  // ListView<Quest*>

  void showAllQuests();
  void showInProgressQuests();
  void showCompletedQuests();

  static std::string generateDesc(const Quest* q);

 private:
  PauseMenu* _pauseMenu;
  ax::Label* _descLabel;
};

}  // namespace vigilante

#endif  // VIGILANTE_QUEST_LIST_VIEW_H_
