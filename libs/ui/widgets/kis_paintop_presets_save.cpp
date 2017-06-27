/* This file is part of the KDE project
 * Copyright (C) 2017 Scott Petrovic <scottpetrovic@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "widgets/kis_paintop_presets_save.h"
#include <QDebug>
#include <QDate>
#include <QTime>


#include <KoFileDialog.h>
#include "KisImportExportManager.h"
#include "QDesktopServices"
#include "kis_resource_server_provider.h"


KisPresetSaveWidget::KisPresetSaveWidget(QWidget * parent)
    : KisPaintOpPresetSaveDialog(parent)
{
    // this is setting the area we will "capture" for saving the brush preset. It can potentially be a different
    // area that the entire scratchpad
    this->brushPresetThumbnailWidget->setCutoutOverlayRect(QRect(0, 0, brushPresetThumbnailWidget->height(), brushPresetThumbnailWidget->width()));


    // we will default to reusing the previous preset thumbnail
    // have that checked by default, hide the other elements, and load the last preset image
    connect(clearBrushPresetThumbnailButton, SIGNAL(clicked(bool)), brushPresetThumbnailWidget, SLOT(fillDefault()));
    connect(useExistingThumbnailCheckbox, SIGNAL(clicked(bool)), this, SLOT(usePreviousThumbnail(bool)));
    connect(loadImageIntoThumbnailButton, SIGNAL(clicked(bool)), this, SLOT(loadImageFromFile()));

    connect(savePresetButton, SIGNAL(clicked(bool)), this, SLOT(savePreset()));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(close()));
}

KisPresetSaveWidget::~KisPresetSaveWidget()
{

}

void KisPresetSaveWidget::scratchPadSetup(KisCanvasResourceProvider* resourceProvider)
{
    m_resourceProvider = resourceProvider;

    this->brushPresetThumbnailWidget->setupScratchPad(m_resourceProvider, Qt::white);
}

void KisPresetSaveWidget::showDialog()
{
    setModal(true);

    // set the name of the current brush preset area.
    KisPaintOpPresetSP preset = m_resourceProvider->currentPreset();

    // UI will look a bit different if we are saving a new brush
    if (m_isSavingNewBrush) {
           this->setWindowTitle(i18n("Save New Brush Preset"));
           this->newBrushNameLabel->setVisible(true);
           this->newBrushNameTexField->setVisible(true);
           this->useExistingThumbnailCheckbox->setVisible(false);
           this->clearBrushPresetThumbnailButton->setVisible(true);
           this->loadImageIntoThumbnailButton->setVisible(true);
           this->currentBrushNameLabel->setVisible(false);

           if (preset) {
               this->newBrushNameTexField->setText(preset->name().append(" ").append(i18n("Copy")));
           }

           this->brushPresetThumbnailWidget->allowPainting(true); // in case it was disabled with normal saving last time

    } else {
        this->setWindowTitle(i18n("Save Brush Preset"));

        if (preset) {
            this->currentBrushNameLabel->setText(preset->name());
        }

        this->newBrushNameLabel->setVisible(false);
        this->newBrushNameTexField->setVisible(false);
        this->useExistingThumbnailCheckbox->setVisible(true);
        this->currentBrushNameLabel->setVisible(true);

        this->useExistingThumbnailCheckbox->setChecked(true);
        usePreviousThumbnail(true);
    }

    show();
}

void KisPresetSaveWidget::usePreviousThumbnail(bool usePrevious)
{

    // hide other elements if we are using the previous thumbnail
    this->clearBrushPresetThumbnailButton->setVisible(!usePrevious);
    this->loadImageIntoThumbnailButton->setVisible(!usePrevious);

    // load the previous thumbnail if we are using the existing one
    if (usePrevious) {
        this->brushPresetThumbnailWidget->paintPresetImage();
    } else {
        brushPresetThumbnailWidget->fillDefault(); // fill with white if we want a new preview area
    }

    this->brushPresetThumbnailWidget->allowPainting(!usePrevious); // don't allow drawing if we are using the existing preset
}

void KisPresetSaveWidget::loadImageFromFile()
{
    // create a dialog to retrieve an image file.
    KoFileDialog dialog(0, KoFileDialog::OpenFile, "OpenDocument");
    dialog.setMimeTypeFilters(KisImportExportManager::mimeFilter(KisImportExportManager::Import));
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    QString filename = dialog.filename(); // the filename() returns the entire path & file name, not just the file name


    if (filename != "") { // empty if "cancel" is pressed
        // take that file and load it into the thumbnail are
        const QImage imageToLoad(filename);

        brushPresetThumbnailWidget->fillTransparent(); // clear the background in case our new image has transparency
        brushPresetThumbnailWidget->paintCustomImage(imageToLoad);
    }

}

void KisPresetSaveWidget::setFavoriteResourceManager(KisFavoriteResourceManager * favManager)
{
    m_favoriteResourceManager = favManager;
}


void KisPresetSaveWidget::savePreset()
{
    KisPaintOpPresetSP curPreset = m_resourceProvider->currentPreset();
    if (!curPreset)
        return;

    m_favoriteResourceManager->setBlockUpdates(true);

    KisPaintOpPresetSP oldPreset = curPreset->clone();
    oldPreset->load();
    KisPaintOpPresetResourceServer * rServer = KisResourceServerProvider::instance()->paintOpPresetServer();
    QString saveLocation = rServer->saveLocation();

    // if we are saving a new brush, use what we type in for the input
    QString presetName = m_isSavingNewBrush ? newBrushNameTexField->text() : curPreset->name();

    QString currentPresetFileName = saveLocation + presetName + curPreset->defaultFileExtension();
    if (rServer->resourceByName(presetName)) {
        QString currentDate = QDate::currentDate().toString(Qt::ISODate);
        QString currentTime = QTime::currentTime().toString(Qt::ISODate);
        QString presetFilename = saveLocation + presetName + "_backup_" + currentDate + "-" + currentTime + oldPreset->defaultFileExtension();
        oldPreset->setFilename(presetFilename);
        oldPreset->setName(presetName);
        oldPreset->setPresetDirty(false);
        oldPreset->setValid(true);
        rServer->addResource(oldPreset);
        QStringList tags;
        tags = rServer->assignedTagsList(curPreset.data());
        rServer->removeResourceAndBlacklist(oldPreset.data());
        Q_FOREACH (const QString & tag, tags) {
            rServer->addTag(oldPreset.data(), tag);
        }
    }

    if (m_isSavingNewBrush) {

        KisPaintOpPresetSP newPreset = curPreset->clone();
        newPreset->setFilename(currentPresetFileName);
        newPreset->setName(presetName);
        newPreset->setImage(brushPresetThumbnailWidget->cutoutOverlay());
        newPreset->setPresetDirty(false);
        newPreset->setValid(true);
        rServer->addResource(newPreset);
        curPreset = newPreset; //to load the new preset

    } else {

        if (curPreset->filename().contains(saveLocation)==false || curPreset->filename().contains(presetName)==false) {
            rServer->removeResourceAndBlacklist(curPreset.data());
            curPreset->setFilename(currentPresetFileName);
            curPreset->setName(presetName);
        }

        if (!rServer->resourceByFilename(curPreset->filename())){
            //this is necessary so that we can get the preset afterwards.
            rServer->addResource(curPreset, false, false);
            rServer->removeFromBlacklist(curPreset.data());
        }

        curPreset->setImage(brushPresetThumbnailWidget->cutoutOverlay());
        curPreset->save();
        curPreset->load();
    }


    // HACK ALERT! the server does not notify the observers
    // automatically, so we need to call theupdate manually!
    rServer->tagCategoryMembersChanged();

    m_favoriteResourceManager->setBlockUpdates(false);


    close(); // we are done... so close the save brush dialog

}

void KisPresetSaveWidget::isSavingNewBrush(bool newBrush)
{
    m_isSavingNewBrush = newBrush;
}


#include "moc_kis_paintop_presets_save.cpp"