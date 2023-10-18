#include "developwindow.h"
#include "ui_developwindow.h"
#include "Protocol/equiptype.h"
#include "ClientGUI/clientv2.h"

extern std::unique_ptr<QSettings> settings;

DevelopWindow::DevelopWindow(QWidget *parent)
    : QDialog{parent},
    ui(new Ui::DevelopWindow) {
    ui->setupUi(this);
    for(auto &equipType: EquipType::allEquipTypes()) {
        ui->listType->addItem(equipType);
    }
    ui->listType->addItem("All equipments");
    resetListName(0);
    connect(ui->listType, &QComboBox::currentIndexChanged,
            this, &DevelopWindow::resetListName);
}

DevelopWindow::~DevelopWindow() {
    delete ui;
}

int DevelopWindow::equipIdDesired() {
    if(!ui->idText->toPlainText().isEmpty())
        return ui->idText->toPlainText().toInt(nullptr, 0);
    else {
        for(auto &equipReg:
             Clientv2::getInstance().equipRegistryCache) {
            for(auto &name: equipReg->localNames) {
                if(name.compare(ui->listName->currentText(),
                                 Qt::CaseInsensitive) == 0) {
                    return equipReg->getId();
                }
            }
        }
        return 0;
    }
}

void DevelopWindow::resetListName(int equiptypeInt) {
    ui->listName->clear();
    for(auto &equipReg:
         Clientv2::getInstance().equipRegistryCache) {
        if(ui->listType->currentText().compare("All equipments") == 0
            || equipReg->type.toString()
                       .compare(ui->listType->currentText(),
                                Qt::CaseInsensitive) == 0) {
            QString equipName = equipReg->toString(
                settings->value("language", "ja_JP").toString());
            if(equipName.isEmpty()) {
                equipName = equipReg->toString("ja_JP");
            }
            ui->listName->addItem(equipName);
        }
    }
}
