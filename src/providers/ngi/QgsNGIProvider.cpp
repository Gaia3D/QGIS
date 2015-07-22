#include "QgsNGIProvider.h"

#include <QgsSpatialIndex.h>
#include <QFile>
#include <QTextStream>

#include "qgslogger.h"
#include "qgsmessagelog.h"

#include <Qgis.h>

#include "QgsNGIDataSource.h"
#include "QgsNGILayerData.h"

#include "QgsNGIFeatureIterator.h"


static const QString NGI_PROVIDER_KEY = "ngi";
static const QString NGI_PROVIDER_DESCRIPTION = QString( "NGI data provider" );


QgsNGIProvider::QgsNGIProvider( QString const & uri )
:	mValid(false)
,	mRelevantFieldsForNextFeature(false)
,	mSpatialIndex(0)
,	mNgiLayer(0)
,	mIsUpdated(true)
{
	mDriverName = "NGI File";

// 	QSettings settings;

	// make connection to the data source

	QgsDebugMsg( "Data source uri is " + uri );

	// try to open for update, but disable error messages to avoid a
	// message if the file is read only, because we cope with that
	// ourselves.

	// This part of the code parses the uri transmitted to the ogr provider to
	// get the options the client wants us to apply

	// If there is no & in the uri, then the uri is just the filename. The loaded
	// layer will be layer 0.
	//this is not true for geojson

	mLayerIndex = -1;
	mLayerName = QString::null;
	if ( !uri.contains( '|', Qt::CaseSensitive ) )
	{
		mFilePath = uri;
	}
	else
	{
		QStringList theURIParts = uri.split( "|" );
		mFilePath = theURIParts.at( 0 );

		for ( int i = 1 ; i < theURIParts.size(); i++ )
		{
			QString part = theURIParts.at( i );
			int pos = part.indexOf( "=" );
			QString field = part.left( pos );
			QString value = part.mid( pos + 1 );

			if ( field == "layerid" )
			{
				bool ok;
				mLayerIndex = value.toInt( &ok );
				if ( ! ok )
				{
					mLayerIndex = -1;
				}
			}
			else if ( field == "layername" || field == "layerindex")
			{
				mLayerName = value;
			}

			if ( field == "subset" )
			{
				mSubsetString = value;
			}

// 			if ( field == "geometrytype" )
// 			{
// 				mOgrGeometryTypeFilter = ogrWkbGeometryTypeFromName( value );
// 			}
		}
	}

	bool openReadOnly = false;


	QgsDebugMsg( "mFilePath: " + mFilePath );
	QgsDebugMsg( "mLayerIndex: " + QString::number( mLayerIndex ) );
	QgsDebugMsg( "mLayerName: " + mLayerName );
	QgsDebugMsg( "mSubsetString: " + mSubsetString );

	setEncoding( "UTF-8" );
	// first try to open in update mode (unless specified otherwise)
	mNgiSource = open(mFilePath);

	if ( mNgiSource )
	{
		QgsDebugMsg( "NGI opened using Driver " + NGI_PROVIDER_KEY );

		// We get the layer which was requested by the uri. The layername
		// has precedence over the layerid if both are given.
		if ( mLayerName.isNull() )
		{
			mNgiOrigLayer = mNgiSource->getLayerByIndex(mLayerIndex);
		}
		else
		{
			mNgiOrigLayer = mNgiSource->getLayerByName(mLayerName);
		}

		mNgiLayer = mNgiOrigLayer;
		if ( mNgiLayer )
		{
			// check that the initial encoding setting is fit for this layer
			setEncoding( "UTF-8" );

			bool valid = setSubsetString( mSubsetString );
			QgsDebugMsg( "Data source is valid" );
		}
		else
		{
			QgsMessageLog::logMessage( tr( "Data source is invalid, no layer found (%1)" ), tr( "NGI" ) );

			mNgiLayer = mNgiSource->getDefaultLayer();

			QgsMessageLog::logMessage( tr( "Default first layer set (%1)" ), mNgiLayer->getName() );
		}

		if(mLayerIndex == -1)
			mLayerIndex = mNgiLayer->getIndex();

		if(mLayerName.isNull())
			mLayerName = mNgiLayer->getName();

		setDataSourceUri( mFilePath );
	}
	else
	{
		QgsMessageLog::logMessage( tr( "Data source is invalid ") , tr( "NGI" ) );
	}

	mNativeTypes
		<< QgsVectorDataProvider::NativeType( tr( "Whole number (integer)" ), "integer", QVariant::Int, 1, 10 )
		<< QgsVectorDataProvider::NativeType( tr( "Decimal number (real)" ), "double", QVariant::Double, 1, 20, 0, 15 )
		<< QgsVectorDataProvider::NativeType( tr( "Text (string)" ), "string", QVariant::String, 1, 255 )
		<< QgsVectorDataProvider::NativeType( tr( "Date" ), "date", QVariant::Date, 8, 8 );

	// Some drivers do not support datetime type
	// Please help to fill this list
// 	if ( mDriverName != "NGI File" )
// 	{
// 		mNativeTypes
// 			<< QgsVectorDataProvider::NativeType( tr( "Date & Time" ), "datetime", QVariant::DateTime );
// 	}
}

QgsNGIProvider::~QgsNGIProvider()
{
	while ( !mActiveIterators.empty() )
	{
		QgsNGIFeatureIterator *it = *mActiveIterators.begin();
		QgsDebugMsg( "closing active iterator" );
		it->close();
	}

	delete mSpatialIndex;
}



int QgsNGIProvider::capabilities() const
{
	return QgsVectorDataProvider::NoCapabilities;
// 	return AddFeatures | DeleteFeatures | ChangeGeometries |
// 		ChangeAttributeValues | AddAttributes | DeleteAttributes | CreateSpatialIndex |
// 		SelectAtId | SelectGeometryAtId;
}

QgsNGIDataSource* QgsNGIProvider::open(QString& filePath)
{
	QgsNGIDataSource* ngiDataSource = new QgsNGIDataSource(this);

	if(ngiDataSource->load(filePath) == true)
	{
		return ngiDataSource;
	}

	return NULL;	
}


QgsRectangle QgsNGIProvider::extent()
{
	if(mIsUpdated)
	{

	}

	return mNgiLayer->getExtent();
// 	if (mExtentRect.isEmpty())
// 	{
// 		// get the extent_ (envelope) of the layer
// 		QgsDebugMsg( "Starting get extent" );
// 
// 		// TODO: This can be expensive, do we really need it!
// 		if ( mNgiLayer == mNgiOrigLayer )
// 		{
// 			return this->mExtent;
// 		}
// 		else
// 		{
// 			return mNgiLayer->getExtent();
// 		}
// 
// 		QgsDebugMsg( "Finished get extent" );
// 	}
// 
// 	OGREnvelope *ext = static_cast<OGREnvelope *>( extent_ );
// 	mExtentRect.set( ext->MinX, ext->MinY, ext->MaxX, ext->MaxY );
// 	return mExtentRect;
}

QString QgsNGIProvider::storageType() const
{
  // Delegate to the driver loaded in by NGI
  return mDriverName;
}

QgsAbstractFeatureSource* QgsNGIProvider::featureSource() const 
{
	return new QgsNGIFeatureSource((QgsNGIProvider*)this);
}

QgsFeatureIterator QgsNGIProvider::getFeatures( const QgsFeatureRequest& request)
{
	return QgsFeatureIterator(new QgsNGIFeatureIterator(new QgsNGIFeatureSource(this), true, request));
}
// 
// bool QgsNGIProvider::createSpatialIndex()
// {
// 	if ( !mSpatialIndex )
// 	{
// 		mSpatialIndex = new QgsSpatialIndex();
// 
// 		// add existing features to index
// 		for ( QgsFeatureMap::iterator it = mFeatures.begin(); it != mFeatures.end(); ++it )
// 		{
// 			mSpatialIndex->insertFeature( *it );
// 		}
// 	}
// 
// 
// 	return true;
// }

void QgsNGIProvider::setEncoding( const QString& e )
{
#if defined(OLCStringsAsUTF8)
	QSettings settings;
	if (( ogrDriverName == "ESRI Shapefile" && settings.value( "/qgis/ignoreShapeEncoding", true ).toBool() ) || !OGR_L_TestCapability( ogrLayer, OLCStringsAsUTF8 ) )
	{
		QgsVectorDataProvider::setEncoding( e );
	}
	else
	{
		QgsVectorDataProvider::setEncoding( "UTF-8" );
	}
#else
	QgsVectorDataProvider::setEncoding( e );
#endif

}


const QgsFields & QgsNGIProvider::fields() const
{
	QgsNGILayer* layer = mNgiSource->getLayerByName(mLayerName);

	if(layer != NULL)
	{
		return *layer->getLayerData()->getSchema();
	}

	return QgsFields();
}

QGis::WkbType QgsNGIProvider::geometryType() const
{
	if(mNgiLayer)
	{
		return (QGis::WkbType)mNgiLayer->getGeomType();
	}

	return QGis::WKBPoint;
}

size_t QgsNGIProvider::layerCount() const
{
	return mNgiSource->getLayerCount();
}


void QgsNGIProvider::updateExtents()
{
	mIsUpdated = true;
}

long QgsNGIProvider::featureCount() const
{
	return 0;
}

QgsCoordinateReferenceSystem QgsNGIProvider::crs()
{
	
	QgsCoordinateReferenceSystem  srs;

	QString layerName = mFilePath.left( mFilePath.indexOf( ".ngi", Qt::CaseInsensitive ) );
	QFile prjFile( layerName + ".qpj" );
	if ( prjFile.open( QIODevice::ReadOnly ) )
	{
		QTextStream prjStream( &prjFile );
		QString myWktString = prjStream.readLine();
		prjFile.close();

		if ( srs.createFromWkt( myWktString.toUtf8().constData() ) )
			return srs;
	}
	else
	{
		QgsDebugMsg( "no spatial reference found" );
	}

	return srs;
}


QStringList QgsNGIProvider::subLayers() const
{
	
	QgsDebugMsg( "Entered." );
	if ( mGeomType == QGis::WKBUnknown )
	{
		return QStringList();
	}

	if ( !mSubLayerList.isEmpty() )
		return mSubLayerList;

	for ( unsigned int i = 0; i < layerCount() ; i++ )
	{
		QgsNGILayer* layer = mNgiSource->getLayer(i);
 		QGis::WkbType layerGeomType = (QGis::WkbType)layer->getGeomType();
		QString theLayerName = layer->getName();
// 		OGRLayerH layer = OGR_DS_GetLayer( ogrDataSource, i );
// 		OGRFeatureDefnH fdef = OGR_L_GetLayerDefn( layer );
// 		QString theLayerName = FROM8( OGR_FD_GetName( fdef ) );
// 		OGRwkbGeometryType layerGeomType = OGR_FD_GetGeomType( fdef );

		QgsDebugMsg( QString( "layerGeomType = %1" ).arg( layerGeomType ) );

		if ( layerGeomType != QGis::WKBUnknown )
		{
			int theLayerFeatureCount = layer->getLayerData()->getCount();

			QString geom = QGis::featureType( layerGeomType );

			mSubLayerList << QString( "%1:%2:%3:%4" ).arg( i ).arg( theLayerName ).arg( theLayerFeatureCount == -1 ? tr( "Unknown" ) : QString::number( theLayerFeatureCount ) ).arg( geom );
		}
		else
		{
			QgsDebugMsg( "Unknown geometry type, count features for each geometry type" );
			// Add virtual sublayers for supported geometry types if layer type is unknown
			// Count features for geometry types
			QMap<QGis::WkbType, int> fCount;
			// TODO: avoid reading attributes, setRelevantFields cannot be called here because it is not constant
			//setRelevantFields( true, QgsAttributeList() );
			QgsNGILayerData* layerData = layer->getLayerData();
			layerData->rewind();

			
			QgsGeometry*	geometry = NULL;
			while (( geometry = layerData->getNextGeometry() ) )
			{
				if ( !geometry ) continue;

				
				if ( geometry )
				{
					QGis::WkbType gType = (QGis::WkbType)geometry->type();
//					OGRwkbGeometryType gType = ogrWkbSingleFlatten( OGR_G_GetGeometryType( geom ) );
					fCount[gType] = fCount.value( gType ) + 1;
				}
			}

			layerData->rewind();

			// it may happen that there are no features in the layer, in that case add unknown type
			// to show to user that the layer exists but it is empty
			if ( fCount.size() == 0 )
			{
				fCount[QGis::WKBUnknown] = 0;
			}

			foreach ( QGis::WkbType gType, fCount.keys() )
			{
				QString geom = QGis::featureType( gType );

				QString sl = QString( "%1:%2:%3:%4" ).arg( i ).arg( theLayerName ).arg( fCount.value( gType ) ).arg( geom );
				QgsDebugMsg( "sub layer: " + sl );
				mSubLayerList << sl;
			}
		}
	}

	return mSubLayerList;
}
static QString createFileFilter_( QString const &longName, QString const &glob )
{
	return longName + " [NGI] (" + glob.toLower() + " " + glob.toUpper() + ");;";
} // createFileFilter_

QString QgsNGIProvider::fileVectorFilters() const
{
	/**File filters*/
	static QString myFileFilters;

	// if we've already built the supported vector string, just return what
	// we've already built

	if ( myFileFilters.isEmpty() || myFileFilters.isNull() )
	{
		// Grind through all the drivers and their respective metadata.
		// We'll add a file filter for those drivers that have a file
		// extension defined for them; the others, welll, even though
		// theoreticaly we can open those files because there exists a
		// driver for them, the user will have to use the "All Files" to
		// open datasets with no explicitly defined file name extension.
		myFileFilters += createFileFilter_( QObject::tr( "Korean National Geographic Information format" ), "*.ngi" );
	}

	return myFileFilters;
}

QString QgsNGIProvider::databaseDrivers() const
{
	return "";

}
QString QgsNGIProvider::protocolDrivers() const
{
	return "";

}

QString QgsNGIProvider::directoryDrivers() const
{
	return "";

}


bool QgsNGIProvider::isValid()
{
	return mGeomType != QGis::WKBUnknown;
}

    
QString QgsNGIProvider::name() const
{
	return NGI_PROVIDER_KEY;
}


QString QgsNGIProvider::description() const
{
	return NGI_PROVIDER_DESCRIPTION;
}

int QgsNGIProvider::layerIndex()
{
	return this->getDataSource()->getLayerByName(mLayerName)->getIndex();
}


/**
 * Class factory to return a pointer to a newly created
 * QgsOgrProvider object
 */
QGISEXTERN QgsNGIProvider * classFactory( const QString *uri )
{
  return new QgsNGIProvider( *uri );
}



/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return NGI_PROVIDER_KEY;
}


/**
 * Required description function
 */
QGISEXTERN QString description()
{
  return NGI_PROVIDER_DESCRIPTION;
}

/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */

QGISEXTERN bool isProvider()
{
  return true;
}