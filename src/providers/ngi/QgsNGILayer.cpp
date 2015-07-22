#include "QgsNGILayer.h"
#include "QgsNgiLayerData.h"

QgsNGILayer::QgsNGILayer(QgsNGILayerData* layerData, QgsRectangle& extentRect)
:	mLayerData(layerData)
,mExtentRect(extentRect)
{
	//initSchema();

}

QgsNGILayer::~QgsNGILayer(void)
{
	clear();

	
}


int QgsNGILayer::getIndex()
{
	if(mLayerData)
	{
		return mLayerData->getLayerID();
	}

	return -1;
}

QString QgsNGILayer::getName()
{
	if(mLayerData)
	{
		return mLayerData->getLayerName();
	}

	return "";
}


int	QgsNGILayer::getGeomType()
{
	return this->mLayerData->getGeometryType();
}
// 
// bool QgsNGILayer::getNextFeature( QgsFeature& feature , QgsFeatureRequest& request)
// {
// // 	OGRFeature  *poFeature = NULL;
// // 
// // 	/* -------------------------------------------------------------------- */
// // 	/*      Collect a matching list if we have attribute or spatial         */
// // 	/*      indices.  Only do this on the first request for a given pass    */
// // 	/*      of course.                                                      */
// // 	/* -------------------------------------------------------------------- */
// // 	if( (m_poAttrQuery != NULL || m_poFilterGeom != NULL)
// // 		&& mNexFId == 0 && panMatchingFIDs == NULL )
// // 	{
// // 		ScanIndices();
// // 	}
// // 
// // 	/* -------------------------------------------------------------------- */
// // 	/*      Loop till we find a feature matching our criteria.              */
// // 	/* -------------------------------------------------------------------- */
// // 	while( TRUE )
// // 	{
// // 		if( panMatchingFIDs != NULL )
// // 		{
// // 			if( panMatchingFIDs[iMatchingFID] == OGRNullFID )
// // 			{
// // 				return NULL;
// // 			}
// // 
// // 			// Check the shape object's geometry, and if it matches
// // 			// any spatial filter, return it.  
// // 			poFeature = FetchShape(panMatchingFIDs[iMatchingFID]);
// // 
// // 			iMatchingFID++;
// // 
// // 		}
// // 		else
// // 		{
// // 			if( mNexFId >= nTotalShapeCount )
// // 			{
// // 				return NULL;
// // 			}
// // 
// // 			if ( hDBF && DBFIsRecordDeleted( hDBF, mNexFId ) ) {
// // 				poFeature = NULL;
// // 			} else {
// // 				// Check the shape object's geometry, and if it matches
// // 				// any spatial filter, return it.  
// // 				poFeature = FetchShape(mNexFId);
// // 			}
// // 			mNexFId++;
// // 		}
// // 
// // 		if( poFeature != NULL )
// // 		{
// // 			if( poFeature->GetGeometryRef() != NULL )
// // 			{
// // 				poFeature->GetGeometryRef()->assignSpatialReference( poSRS );
// // 			}
// // 
// // 			m_nFeaturesRead++;
// // 
// // 			if( (m_poFilterGeom == NULL || FilterGeometry( poFeature->GetGeometryRef() ) )
// // 				&& (m_poAttrQuery == NULL || m_poAttrQuery->Evaluate( poFeature )) )
// // 			{
// // 				return poFeature;
// // 			}
// // 
// // 			delete poFeature;
// // 		}
// // 	}        
// 
// 	return false;
// }
// 
// bool QgsNGILayer::getFeature( QgsFeature& feature, QgsFeatureId fid , QgsFeatureRequest& request)
// {
// 	throw std::exception("The method or operation is not implemented.");
// }

void QgsNGILayer::clear()
{
	if(mLayerData != NULL)
	{
		delete mLayerData;
		mLayerData = NULL;
	}
}

