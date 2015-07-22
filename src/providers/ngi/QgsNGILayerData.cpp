#include "QgsNGILayerData.h"

#include <Qgslogger.h>
#include <QgsGeometry.h>
#include <QgsSpatialIndex.h>


QgsNGILayerData::QgsNGILayerData(void)
:	mSpatialIndex(0)
{
}

QgsNGILayerData::~QgsNGILayerData(void)
{
	clear();
	
}

void QgsNGILayerData::clear()
{
	QgsFeature* feature;
	QgsAttributes* attrs;
	QgsFeaturesMap::Iterator it = mFeatures.begin();
	while(it != mFeatures.end())
	{
		feature = it.value();
		delete feature;

		it++;
	}

	QgsFeatureAttributeMap::Iterator attrs_it = mFeatureFields.begin();
	while(attrs_it != mFeatureFields.end())
	{
		attrs = attrs_it.value();
		delete attrs;

		attrs_it++;
	}

	if(mSpatialIndex)
		delete mSpatialIndex;
}

bool QgsNGILayerData::rewind()
{
	if(mFeatures.empty())	return false;

	this->mCurrentIterator = mFeatures.begin();

	return true;
}


QgsGeometry* QgsNGILayerData::getNextGeometry()
{
	if(mCurrentIterator != mFeatures.end())
	{
		QgsFeature* feature = mCurrentIterator.value();
		mCurrentIterator++;

		return (QgsGeometry*)feature->geometry();
// 		return geom;
	}

	return NULL;
}

// bool QgsNGILayerData::getFeatureAttribute(QgsFeature& f, qint64 fid, int attindex)
// {
// 	QVariant retVal;
// 
// 	if(!isValidFeatureDefnRef(attindex))
// 	{
// 		QgsDebugMsg( "ogrFet->GetFieldDefnRef(attindex) returns NULL" );
// 		return  false;
// 	}
// 
// 
// 	QgsAttributes* atts = getAttributes(fid);
// 	if(atts == NULL)
// 	{
// 		QgsDebugMsg( "getAttribute(fid) returns NULL" );
// 		return false;
// 	}
// 	
// 
// 	f.setAttribute(attindex, atts->at(attindex));
// 
// 	return true;
// }

bool QgsNGILayerData::appendGeometry(qint64 fid, QgsGeometry* geometry)
{
	if(!isValidGeometry(geometry))
	{
		return false;
	}

	bool retVal = true;
	if(mFeatures.contains(fid))
	{
		retVal = false;
	}

	QgsFeature* feature = new QgsFeature(fid);
	feature->setGeometry(geometry);

	mFeatures[fid] = feature;

	return retVal;
}

bool QgsNGILayerData::appendAttribute(qint64 fid, QgsAttributes* atts)
{
	if(!isValidAttribute(atts))
	{
		QgsDebugMsg("Attribute is in Valid!!");
		return false;
	}
	
	bool retVal = true;
	if(mFeatureFields.contains(fid))
	{
		retVal = false;
	}

	mFeatureFields[fid] = atts;

	return retVal;
}

QgsAttributes* QgsNGILayerData::getAttributes(qint64 fid)
{
	if(!mFeatureFields.contains(fid))
	{
		return NULL;
	}

	return mFeatureFields.value(fid);
}

bool QgsNGILayerData::isValidAttribute(QgsAttributes* atts)
{
	int scount = mFieldSchema.count();
	int acount = atts->count();
	if(mFieldSchema.count() != atts->count())
	{
		return false;
	}

	for(int i=0; i< mFieldSchema.count(); i++)
	{
		QgsField& field = mFieldSchema[i];
		
		if(field.type() != atts->at(i).type())
		{
			return false;
		}
	}

	return true;
}

bool QgsNGILayerData::isValidFeatureDefnRef(int attindex)
{
	if(attindex < 0 || attindex >= mFieldSchema.count())
	{
		return false;
	}

	return true;
}

bool QgsNGILayerData::createSpatialIndex()
{
	if ( !mSpatialIndex )
	{
		mSpatialIndex = new QgsSpatialIndex();

		// add existing features to index
		for ( QgsFeaturesMap::iterator it = mFeatures.begin(); it != mFeatures.end(); ++it )
		{
			mSpatialIndex->insertFeature( * *it );
		}
	}

	return true;
}


void QgsNGILayerData::setLayerName( QString& layername )
{
	mLayerName = layername;
}

void QgsNGILayerData::setLayerID( int layerID )
{
	mLayerID = layerID;
}

bool QgsNGILayerData::isValidGeometry( QgsGeometry* geometry )
{
	return (geometry->wkbType() == this->getGeometryType());
}

