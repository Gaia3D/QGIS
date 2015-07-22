#pragma once


#include <QgsField.h>
#include <QVariant.h>
#include <QMap.h>
#include <QgsRectangle.h>

#include <QgsFeature.h>
#include <QgsRectangle.h>

class QgsNGIFeatureIterator;
class QgsSpatialIndex;

typedef QMap<QgsFeatureId, QgsFeature*>	QgsFeaturesMap;

class QgsNGILayerData
{
public:
	friend class QgsNGIFeatureIterator;

	typedef QMap<qint64, QgsAttributes*>	QgsFeatureAttributeMap;	// feature id, feature field attributes
	typedef QMap<qint64, QgsGeometry*>		QgsGeometryMap;			// feature id, feature geometry

public:

	QgsNGILayerData(void);
	virtual ~QgsNGILayerData(void);

	QgsFields*	getSchema() {return &mFieldSchema;}

	void		setSchema(QgsFields& schemas) { mFieldSchema = schemas;}

	int			getCount() { return mFeatures.size();}

	bool		rewind();
	
	QgsGeometry*	getNextGeometry();

	QgsAttributes* getAttributes(qint64 fid);

	bool		appendAttribute(qint64 fid, QgsAttributes* atts);

	bool		appendGeometry(qint64 fid, QgsGeometry* geometry);

	void		setGeometryType(QGis::WkbType type){ mGeomType = type; }

	QGis::WkbType	getGeometryType(){return mGeomType; }

	void		setLayerName( QString& layername );
	void		setLayerID( int layerID );

	QString		getLayerName() { return mLayerName; }
	int			getLayerID() { return mLayerID; }

	bool		hasAttributes() { return !mFeatureFields.empty(); }

    /**
     * Creates a spatial index
     * @return true in case of success
     */
    bool createSpatialIndex();


private:

	bool isValidFeatureDefnRef(int attindex);

	bool isValidAttribute(QgsAttributes* atts);

	
	bool isValidGeometry( QgsGeometry* geometry );

	void clear();

protected:

	QgsFields				mFieldSchema;

	QGis::WkbType			mGeomType;

//	QgsRectangle			mExtent;

	QgsFeatureAttributeMap	mFeatureFields;

	QgsFeaturesMap::iterator	mCurrentIterator;

	QgsFeaturesMap			mFeatures;

	QgsSpatialIndex*		mSpatialIndex;

	QString					mLayerName;
	
	int						mLayerID;
};
