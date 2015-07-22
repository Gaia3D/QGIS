#pragma once


#include <qgis.h>
#include "QIODevice"
#include "QTextCodec"

#include <QgsGeometry.h>

#include "qgsfield.h"

#include "qgsrectangle.h"

#include "QgsNGILayer.h"


class QgsNGIProvider;
class QTextStream;
class QgsNGILayerData;

typedef QMap<int, QgsAttributes> QgsFeatureAttributeMap;	// geometry id , fields
typedef QVector<QVariant>  QgsAttributes;			// field values

struct QgsNgiFieldInfo
{
	QgsNgiFieldInfo():fields(0){};
	QgsFields*		fields;
	int				featureCount;
};

class QgsNGIDataSource
{
public:

	QgsNGIDataSource(QgsNGIProvider* provider);
	~QgsNGIDataSource(void);

// 	QString getName();

	bool load(QString ngiFile);

	QgsNGILayer* getLayerData( int idx );

	size_t getLayerCount();

	QgsNGILayer* getDefaultLayer();

	QgsNgiFieldInfo* getFieldInfo(QString layername);

	QgsNGILayer* getLayerByName( QString layername );
	QgsNGILayer* getLayerByIndex( int index );

	QgsNGILayer* getLayer( int index );

	QGis::WkbType getWkbTypeByName( QString& shapeType );

protected:

	bool createLayer(QTextStream* in);
	void loadSchemas( QString ndaFilename );
// 	bool createSchema( QTextStream* InStream, QgsNgiFieldInfo* finfo , QgsFeatureAttributeMap* fields);
	
	void setFields( QStringList& list, QgsNgiFieldInfo* finfo);

// 	void readRecordData(QString& line, QgsNgiFieldInfo* finfo, QgsFeatureAttributeMap* fields);


	bool createSchema( QTextStream* InStream, QgsNGILayer* layer);
	
	void setFields( QStringList& list, QgsFields* fields);

	void readRecordData(QString& line, QgsNGILayer* layer);

	void loadFeatures(QTextStream* in, QgsNGILayerData* layerData);
	QgsGeometry* loadFeature(QTextStream* in);

	QgsPoint loadPoint(QTextStream* in);
	QgsPolygon loadPolygon(QTextStream* in);
	QgsPolyline loadPolyline(QTextStream* in);
	void clear();
	void clearSchemas();

private:

	QMap<QString, QgsNGILayer*>		mLayers;
	QMap<QString, QgsNgiFieldInfo*>	mLayerSchemas;
	QMap<QString, QgsFeatureAttributeMap*>	mLayerFields;

	QgsNGILayer*				mLayer;
	bool						mIsFirstLayer;

	QString						mFilename;
	QTextStream*				mInStream;	
	QgsNGIProvider*				mProvider;
};
