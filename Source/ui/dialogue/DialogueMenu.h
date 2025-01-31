// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#ifndef VIGILANTE_DIALOGUE_MENU_H_
#define VIGILANTE_DIALOGUE_MENU_H_

#include <memory>

#include <axmol.h>
#include <2d/Label.h>
#include <ui/UIImageView.h>
#include "Controllable.h"
#include "ui/dialogue/DialogueListView.h"

namespace vigilante {

class Npc;

class DialogueMenu : public Controllable {
 public:
  DialogueMenu();
  virtual ~DialogueMenu() = default;

  virtual void handleInput() override; // Controllable

  ax::Layer* getLayer() const;
  DialogueListView* getDialogueListView() const;

 private:
  ax::Layer* _layer;

  ax::ui::ImageView* _background;
  std::unique_ptr<DialogueListView> _dialogueListView;
};

}  // namespace vigilante

#endif  // VIGILANTE_DIALOGUE_MENU_H_
