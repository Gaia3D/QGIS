#include "QgsNGIFeatureIterator.h"

#include <QgsExpression.h>
#include <QgsSpatialIndex.h>

#include "QgsNGIProvider.h"
#include "QgsNGIDataSource.h"
#include "QgsNgiLayerData.h"

QgsNGIFeatureIterator::QgsNGIFeatureIterator(QgsNGIFeatureSource* source, bool ownSource, const QgsFeatureRequest& request )
    : QgsAbstractFeatureIteratorFromSource<QgsNGIFeatureSource>( source, ownSource, request  )
//:QgsAbstractFeatureIterator(request)
, mDataSource(0)
, mLayer(0)
, mSelectRectGeom(0)
, mExp(0)
, mSubsetStringSet( false )
{
	mFeatureFetched = true;

	QgsNGIProvider* p = source->getProvider();

	mDataSource = p->getDataSource();

	mProvider = p;

	if ( p->layerName().isNull() )
	{
		mLayer = mDataSource->getLayerByIndex(p->layerIndex());
	}
	else
	{
		mLayer = mDataSource->getLayerByName(p->layerName());
	}

	if ( !p->subsetString().isEmpty() )
	{
// 		ogrLayer = p->setSubsetString( mLayer, mDataSource );
		mSubsetStringSet = setSubsetString(p->subsetString());

	}

	mLayerData = mLayer->getLayerData();
	mLayerData->createSpatialIndex();


// 	ensureRelevantFields();


	if ( mRequest.filterType() == QgsFeatureRequest::FilterRect && mRequest.flags() & QgsFeatureRequest::ExactIntersect )
	{
		mSelectRectGeom = QgsGeometry::fromRect( request.filterRect() );
	}


	// spatial query to select features
	if ( mRequest.filterType() == QgsFeatureRequest::FilterRect )
	{
		QString wktExtent = QString( "POLYGON((%1))" ).arg( mRequest.filterRect().asPolygon() );
	
		QgsDebugMsg("spatial filter rect : " + wktExtent);
		this->mFeatureIdList = mLayerData->mSpatialIndex->intersects(mRequest.filterRect());
	}
	else if(mRequest.filterType() == QgsFeatureRequest::FilterFid )
	{
		mUsingFeatureIdList = true;

		QgsFeaturesMap::Iterator it = mLayerData->mFeatures.find(mRequest.filterFid());
		if(it != mLayerData->mFeatures.end())
		{

			mFeatureIdList.append(mRequest.filterFid());
		}
	}
	else{
		mUsingFeatureIdList = false;
	}

	//start with first feature
	rewind();
}



QgsNGIFeatureIterator::~QgsNGIFeatureIterator(void)
{
	close();
}


//bool QgsNGIFeatureIterator::setSubsetString(QString& )
bool QgsNGIFeatureIterator::rewind()
{
	if ( mClosed )
		return false;

	if ( mUsingFeatureIdList )
		mFeatureIdListIterator = mFeatureIdList.begin();
	else
		mSelectIterator = mLayerData->mFeatures.begin();

	return true;
}


bool QgsNGIFeatureIterator::close()
{
	if ( mClosed )
		return false;

	iteratorClosed();

	if(mSelectRectGeom)
	{
		delete mSelectRectGeom;
		mSelectRectGeom = NULL;
	}

	if(mExp)
	{
		delete mExp;
		mExp = 0;
	}

	mClosed = true;
	return true;
}

// void QgsNGIFeatureIterator::ensureRelevantFields()
// {
// 	bool needGeom = ( mRequest.filterType() == QgsFeatureRequest::FilterRect ) || !( mRequest.flags() & QgsFeatureRequest::NoGeometry );
// 	QgsAttributeList attrs = ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes ) ? mRequest.subsetOfAttributes() : mProvider->attributeIndexes();
// 	mProvider->setRelevantFields( needGeom, attrs );
// 	mProvider->mRelevantFieldsForNextFeature = true;
// }

bool QgsNGIFeatureIterator::fetchFeature( QgsFeature& feature )
{
	feature.setValid( false );

	if ( mClosed )
		return false;

	if ( mUsingFeatureIdList )
		return nextFeatureUsingList( feature );
	else
		return nextFeatureTraverseAll( feature );
}



bool QgsNGIFeatureIterator::nextFeatureUsingList( QgsFeature& feature )
{
	bool hasFeature = false;

	// option 1: we have a list of features to traverse
	while ( mFeatureIdListIterator != mFeatureIdList.end() )
	{
		if ( mRequest.filterType() == QgsFeatureRequest::FilterRect && mRequest.flags() & QgsFeatureRequest::ExactIntersect )
		{
			// do exact check in case we're doing intersection
			if (mLayerData->mFeatures[*mFeatureIdListIterator]->geometry()->intersects( mSelectRectGeom ) )
				hasFeature = true;
		}
		else
			hasFeature = true;

		if ( hasFeature && filterEval(*mLayerData->mFeatures[*mFeatureIdListIterator]) == true)
			break;

		hasFeature = false;
		mFeatureIdListIterator++;
	}

	// copy feature
	if ( hasFeature )
	{
		feature = *mLayerData->mFeatures[*mFeatureIdListIterator];
		feature.setValid(true);
		mFeatureIdListIterator++;
	}
	else
		close();

	if ( hasFeature )
		feature.setFields( &mProvider->fields()); // allow name-based attribute lookups

	return hasFeature;
}


bool QgsNGIFeatureIterator::nextFeatureTraverseAll( QgsFeature& feature )
{
	bool hasFeature = false;

	// option 2: traversing the whole layer
	while ( mSelectIterator != mLayerData->mFeatures.end() )
	{
		if ( mRequest.filterType() != QgsFeatureRequest::FilterRect )
		{
			// selection rect empty => using all features
			hasFeature = true;
		}
		else
		{
			if ( mRequest.flags() & QgsFeatureRequest::ExactIntersect )
			{
				// using exact test when checking for intersection
				if ( mSelectIterator.value()->geometry()->intersects( mSelectRectGeom ) )
					hasFeature = true;
			}
			else
			{
				// check just bounding box against rect when not using intersection
				if (mSelectIterator.value()->geometry()->boundingBox().intersects( mRequest.filterRect() ) )
					hasFeature = true;
			}
		}

		if ( hasFeature && filterEval(*mSelectIterator.value()) == true)
			break;

		hasFeature = false;

		mSelectIterator++;
	}

	// copy feature
	if ( hasFeature )
	{
		feature = *mSelectIterator.value();
		qint64 featureId = mSelectIterator.key();
		feature.setFeatureId(featureId);

		QgsFields* fields = mLayerData->getSchema();
		feature.setFields( fields, true); // allow name-based attribute lookups
		setFeatureAttributes(feature);

		mSelectIterator++;
		feature.setValid( true );
	}
	else
		close();

	return hasFeature;
}

void QgsNGIFeatureIterator::setFeatureAttributes(QgsFeature& feature)
{
	if(mLayerData->hasAttributes() == false)	return;

	QVariant value;
	QgsAttributes* attrs = mLayerData->getAttributes(feature.id());


// 	for(int i=0; i<attrs->size(); i++)
// 	{
// 		value = attrs->at(i);
// 		feature.setAttribute(i, value);
// 	}

 	feature.setAttributes(*attrs);
}
 
bool QgsNGIFeatureIterator::setSubsetString( QString& expr )
{
	QgsExpression* exp = 0;

//	QgsDebugMsg("hasAttributes : " + mLayerData->hasAttributes() + );

	if(mLayerData->hasAttributes() == false)
	{
		QgsDebugMsg("layer : " + mLayerData->getLayerName() + " has no Attributes");
		return false;
	}

	try
	{
		exp = new QgsExpression(expr);
		if(exp->hasParserError() == true)
		{
			delete exp;
			return false;
		}

		exp->prepare(this->mProvider->fields());

		mExp = exp;
	}
	catch (QgsException* e)
	{
		delete exp;
		QgsDebugMsg(e->what());
		
		return false;
	}


	return true;
}

bool QgsNGIFeatureIterator::filterEval( QgsFeature& feature )
{
	if(mExp == 0)	return true;

	QVariant value;
	try
	{
		value = mExp->evaluate(feature);
		return value.toBool();
	}
	catch (QgsException* e)
	{
		QgsDebugMsg(e->what());

		return false;
	}
}

    

// -------------------------

QgsNGIFeatureSource::QgsNGIFeatureSource( QgsNGIProvider* p )
    : mProvider( p )
{
}

QgsNGIFeatureSource::~QgsNGIFeatureSource()
{
}

QgsFeatureIterator QgsNGIFeatureSource::getFeatures( const QgsFeatureRequest& request )
{
  return QgsFeatureIterator( new QgsNGIFeatureIterator( this, false, request ) );
}
    

