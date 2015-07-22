#pragma once

#include "qgis.h"

#include <QString>
#include <QVariant>
#include <QgsRectangle.h>

#include <QgsFeature.h>
#include <QgsFeatureRequest.h>

class QTextStream;
class QFile;
class QgsNGILayerData;

class QgsNGILayer
{
public:
	QgsNGILayer(QgsNGILayerData* layerData, QgsRectangle& extentRect);
	virtual ~QgsNGILayer(void);

	int		getGeomType();

 	QgsRectangle	getExtent(){return this->mExtentRect;}

	int				getIndex();

	QString			getName();

	QgsNGILayerData* getLayerData() { return mLayerData; }

	void clear();

private:


private:

	QgsNGILayerData*	mLayerData;
	QgsRectangle		mExtentRect;
	
	bool				mbInit;
};
