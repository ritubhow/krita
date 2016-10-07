/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_bmp_export.h"

#include <QCheckBox>
#include <QSlider>

#include <kpluginfactory.h>
#include <QFileInfo>
#include <QApplication>

#include <KisMimeDatabase.h>
#include <KisExportCheckRegistry.h>
#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>

K_PLUGIN_FACTORY_WITH_JSON(KisBMPExportFactory, "krita_bmp_export.json", registerPlugin<KisBMPExport>();)

KisBMPExport::KisBMPExport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisBMPExport::~KisBMPExport()
{
}

KisImportExportFilter::ConversionStatus KisBMPExport::convert(KisDocument *document, QIODevice *io, KisPropertiesConfigurationSP /*configuration*/)
{
    QRect rc = document->image()->bounds();
    QImage image = document->image()->projection()->convertToQImage(0, 0, 0, rc.width(), rc.height(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
    image.save(io, QFileInfo(filename()).suffix().toLatin1());
    return KisImportExportFilter::OK;
}

void KisBMPExport::initializeCapabilities()
{

    QList<QPair<KoID, KoID> > supportedColorModels;
    supportedColorModels << QPair<KoID, KoID>()
            << QPair<KoID, KoID>(RGBAColorModelID, Integer8BitsColorDepthID);
    addSupportedColorModels(supportedColorModels, KisMimeDatabase::descriptionForMimeType(mimeType()));
    addCapability(KisExportCheckRegistry::instance()->get("ColorModelPerLayerCheck/" + RGBAColorModelID.id() + "/" + Integer8BitsColorDepthID.id())->create(KisExportCheckBase::SUPPORTED));
}

#include "kis_bmp_export.moc"

