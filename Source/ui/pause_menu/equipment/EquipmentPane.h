// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#ifndef VIGILANTE_EQUIPMENT_PANE_H_
#define VIGILANTE_EQUIPMENT_PANE_H_

#include <memory>
#include <string>
#include <vector>

#include <axmol.h>
#include <2d/Label.h>
#include <ui/UILayout.h>
#include <ui/UIImageView.h>

#include "input/InputManager.h"
#include "item/Equipment.h"
#include "ui/pause_menu/AbstractPane.h"
#include "ui/TableLayout.h"

namespace vigilante {

class EquipmentPane : public AbstractPane {
 public:
  explicit EquipmentPane(PauseMenu* pauseMenu);
  virtual ~EquipmentPane() = default;

  virtual void update() override;
  virtual void handleInput() override;

  void selectUp();
  void selectDown();
  void confirm();

  Equipment* getSelectedEquipment() const;
  Equipment::Type getSelectedEquipmentType() const;

 private:
  class EquipmentItem {
   public:
    EquipmentItem(EquipmentPane* parent, const std::string& title, float x, float y);
    virtual ~EquipmentItem() = default;

    inline Equipment* getEquipment() const { return _equipment; }
    void setEquipment(Equipment* equipment);

    void setSelected(bool selected) const;
    inline ax::ui::Layout* getLayout() const { return _layout; }

   private:
    static const int _kEquipmentIconSize;

    EquipmentPane* _parent;
    TableLayout* _layout;
    ax::ui::ImageView* _background;
    ax::ui::ImageView* _icon;
    ax::Label* _equipmentTypeLabel;
    ax::Label* _equipmentNameLabel;
    Equipment* _equipment;

    friend class EquipmentPane;
  };

  std::vector<std::unique_ptr<EquipmentItem>> _equipmentItems;
  int _current;
};

} // namespace vigilante

#endif // VIGILANTE_EQUIPMENT_PANE_H_
