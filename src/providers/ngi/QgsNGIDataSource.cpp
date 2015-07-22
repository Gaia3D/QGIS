#include "QgsNGIDataSource.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>


#include <qgslogger.h>

#include "QgsNGIProvider.h"

#include "QgsNGILayer.h"
#include "QgsNGILayerData.h"

QgsNGIDataSource::QgsNGIDataSource(QgsNGIProvider* provider)
:mInStream(0)
,mProvider(provider)
,mLayer(0)
,mIsFirstLayer(true)
{
}


QgsNGIDataSource::~QgsNGIDataSource(void)
{
	clear();
}

void QgsNGIDataSource::clearSchemas()
{
	if(!mLayerSchemas.empty())
	{
		QgsNgiFieldInfo* finfo;
		QMap<QString, QgsNgiFieldInfo*>::iterator it = mLayerSchemas.begin();
		for(it; it != mLayerSchemas.end(); it++)
		{
			finfo = it.value();
			if(finfo->fields != NULL)
			{
				delete finfo->fields;
			}

			delete finfo;
		}

		mLayerSchemas.clear();
	}
}

void QgsNGIDataSource::clear()
{
	if(!mLayers.empty())
	{
		QgsNGILayer* layer;
		QMap<QString, QgsNGILayer*>::iterator it = mLayers.begin();
		for(it; it != mLayers.end(); it++)
		{
			layer = it.value();
			delete layer;
		}

		mLayers.clear();
	}

	if(!mLayerFields.empty())
	{
		QgsFeatureAttributeMap* atts;
		QMap<QString, QgsFeatureAttributeMap*>::iterator it = mLayerFields.begin();
		for(it; it != mLayerFields.end(); it++)
		{
			atts = it.value();
			delete atts;
		}

		mLayerFields.clear();
	}

	clearSchemas();
	
}

// QString QgsNGIDataSource::getName()
// {
// 	return "";
// }


bool QgsNGIDataSource::load(QString ngiFile)
{
	QFile file(ngiFile);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text ))
	{
		QMessageBox::information(0, "error", file.errorString());
	}

	mInStream = new QTextStream(&file); 
	while(!mInStream->atEnd())
	{
		QString line = mInStream->readLine();
		if(line.compare(line, "$LAYER_ID", Qt::CaseInsensitive) == 0)
		{
			createLayer(mInStream);
		}
	}

	QString ndaFilename = ngiFile.left(ngiFile.length()-3) + "nda";

	QFileInfo fileInfo(ndaFilename);
	if(fileInfo.exists() == true)
	{
		loadSchemas(ndaFilename);
	}


	mFilename = ngiFile;
	delete mInStream;
	mInStream = NULL;

	file.close();

	return true;
}

QgsPolyline QgsNGIDataSource::loadPolyline(QTextStream* in)
{
	QStringList coords;
	QString line;

	int nPoints = in->readLine().toInt();
	QgsPolyline pline( nPoints );

	for(int i=0; i<nPoints; i++)
	{
		line = in->readLine();
		coords = line.split(" ");

		pline[i] = QgsPoint(coords.at(0).toDouble(), coords.at(1).toDouble());
	}

	return pline;
}


QgsPolygon QgsNGIDataSource::loadPolygon(QTextStream* in)
{
	QStringList coords;

	QString line = in->readLine();
	int numofParts = line.replace("NUMPARTS", "").toInt();
	QgsPolygon polygon(numofParts);
	for(int part=0; part<numofParts; part++)
	{
		int nPoints = in->readLine().toInt();
		QgsPolyline pline( nPoints + 1);

		for(int i=0; i<nPoints; i++)
		{
			line = in->readLine();
			coords = line.split(" ");

			pline[i] = QgsPoint(coords.at(0).toDouble(), coords.at(1).toDouble());

//			QgsDebugMsg("point : " + pline[i].toString());
		}

		pline[nPoints] = pline[0];

		polygon[part] = pline;
	}

	return polygon;
}

QgsPoint QgsNGIDataSource::loadPoint(QTextStream* in)
{
	QString line = in->readLine();
	QStringList coords = line.split(" ");

	return QgsPoint(coords.at(0).toDouble(), coords.at(1).toDouble());
}


QgsGeometry* QgsNGIDataSource::loadFeature(QTextStream* in)
{
	QString line = in->readLine();

	if (line.startsWith("TEXT") || line.startsWith("POINT"))
	{
		QgsPoint poiint = loadPoint(in);

		return QgsGeometry::fromPoint(poiint);
	} 
	else if (line.startsWith("MULTIPOINT"))
	{
		QStringList coords;

		int nPoints = in->readLine().toInt();
		QgsMultiPoint points( nPoints );
		for(int i=0; i<nPoints; i++)
		{
			points[i] = loadPoint(in);
		}

		return QgsGeometry::fromMultiPoint(points);
	}
	else if (line.startsWith("LINE"))
	{
		QgsPolyline  pline = loadPolyline(in);


		return QgsGeometry::fromPolyline(pline);
	}
	else if (line.startsWith("MULTILINE"))
	{
		line = in->readLine();
		int numofParts = line.replace("NUMPARTS", "").toInt();

		QgsMultiPolyline mplines( numofParts );
		for(int p=0; p<numofParts; p++)
		{
			QgsPolyline pline = loadPolyline(in);
			mplines[p] = pline;
		}

		return QgsGeometry::fromMultiPolyline(mplines);
	}
	else if (line.startsWith("POLYGON"))
	{
		QgsPolygon  poly = loadPolygon(in);

		return QgsGeometry::fromPolygon(poly);
	}
	else if (line.startsWith("MULTIPOLY"))
	{
		line = in->readLine();
		int numofParts = line.replace("NUMPARTS", "").toInt();

		QgsMultiPolygon mpoly( numofParts );
		for(int p=0; p<numofParts; p++)
		{
			QgsPolygon  poly = loadPolygon(in);
			mpoly[p] = poly;
		}

		return QgsGeometry::fromMultiPolygon(mpoly);
	}

	return NULL;
}

void QgsNGIDataSource::loadFeatures(QTextStream* in, QgsNGILayerData* layerData)
{
	if(in == NULL || layerData == NULL)
	{
		return;
	}

	QString line = in->readLine();
	while(!line.startsWith("<LAYER_END>") &&!in->atEnd())
	{
		line = in->readLine();
		if (line.startsWith("$RECORD"))
		{
			qint64 id = line.replace("$RECORD", "").toInt();
			QgsGeometry* geom = loadFeature(in);

			if(geom)
				layerData->appendGeometry(id, geom);
		}
	}
}

bool QgsNGIDataSource::createLayer(QTextStream* in)
{
	try
	{
		QgsNGILayerData* layerData = new QgsNGILayerData();
		QString line = in->readLine();
		int layerID = line.toInt();

		QgsDebugMsg( "start createLayer ID : " + line);

		bool bInit = false, bInitType=false, bInitBound=false;
		qint64 pos;
		QString layerName;
		QgsRectangle extentRect;
		QGis::WkbType type;
		QString shapelist, shapeType;

		while(!in->atEnd())
		{
			line = in->readLine();
			if(bInit == false && line.compare("$LAYER_NAME", Qt::CaseInsensitive) == 0)
			{
				line = in->readLine();
				pos = in->device()->pos();
				layerName = line.mid(1, line.length() - 2);

				QgsDebugMsg( "layerName : " + layerName);
				bInit = true;
			}

			if (bInitType == false && line.compare("$GEOMETRIC_METADATA", Qt::CaseInsensitive) == 0)
			{
				line = in->readLine();
				shapelist = line.mid(5, line.length()-6);

				QgsDebugMsg( "shapelist : " + shapelist);

				if(shapelist.contains("POLYGON", Qt::CaseInsensitive) == true)
				{
					type = QGis::WKBPolygon;
				}
				else 
				{
					int spos = shapelist.indexOf(",");
					if(spos > 0)
					{
						shapeType = shapelist.mid(0, spos);
					}
					else
					{
						shapeType = shapelist;				
					}

					type = getWkbTypeByName(shapeType);
				}

				bInitType = true;
			}

			if(bInitBound == false && line.contains("BOUND(", Qt::CaseInsensitive) == true)
			{
				line = line.mid(6, line.length() - 7);

				QgsDebugMsg( "BOUND : " + line);

				QStringList list = line.split(",");
				double MinX = list.at(0).toDouble();
				double MinY = list.at(1).toDouble();
				double MaxX = list.at(2).toDouble();
				double MaxY = list.at(3).toDouble();
	
				extentRect.set( MinX, MinY, MaxX, MaxY );

				bInitBound = true;

				if(bInitType == true)
					layerData->setGeometryType(type);

				loadFeatures(in, layerData);

				break;
			}

			
		}

		if(bInit == true && bInitBound == true )
		{
// 			layerData->setExtent(extentRect);
			
			layerData->setLayerName(layerName);
			layerData->setLayerID(layerID);
			QgsNGILayer* ngiLayer = new QgsNGILayer(layerData, extentRect);

			this->mLayers.insert(layerName, ngiLayer);
			QgsDebugMsg( "end createLayer : " + layerName);

			if(mIsFirstLayer)
			{
				mLayer = ngiLayer;
				mIsFirstLayer = false;
			}

			return true;
		}

	}
	catch (std::exception* e)
	{
		QgsLogger::fatal(e->what());
	}

	return false;
}

QgsNGILayer* QgsNGIDataSource::getDefaultLayer()
{
	if(mLayer)
	{
		return mLayer;
	}

	return NULL;
}

QgsNGILayer* QgsNGIDataSource::getLayerByName( QString layerName )
{
	QMap<QString, QgsNGILayer*>::ConstIterator ii = mLayers.begin();
	for(ii; ii!= mLayers.end(); ii++)
	{
		QgsNGILayer* layer = (QgsNGILayer*)ii.value();
//		QgsDebugMsg( "layer: " + layer->getName() + " , index : " +  QString().arg(layer->getIndex()) );
		if(layer->getName().compare(layerName, Qt::CaseInsensitive) == 0)
		{
			return layer;
		}
	}

	return NULL;
}

QGis::WkbType QgsNGIDataSource::getWkbTypeByName( QString& shapeType )
{
	if (shapeType.startsWith("TEXT"))
	{
		return QGis::WKBPoint;
	} 
	else if (shapeType.startsWith("POINT"))
	{
		return QGis::WKBPoint;
	}
	else if (shapeType.startsWith("MULTIPOINT"))
	{
		return QGis::WKBMultiPoint;
	}
	else if (shapeType.startsWith("LINESTRING"))
	{
		return QGis::WKBLineString;
	}
	else if (shapeType.startsWith("MULTILINESTRING"))
	{
		return QGis::WKBMultiLineString;
	}
	else if (shapeType.startsWith("MULTILINE"))
	{
		return QGis::WKBMultiLineString;
	}
	else if (shapeType.startsWith("NETWORKCHAIN"))
	{
		return QGis::WKBLineString;
	}
	else if (shapeType.startsWith("NETWORK CHAIN"))
	{
		return QGis::WKBLineString;
	}
	else if (shapeType.startsWith("POLYGON"))
	{
		return QGis::WKBPolygon;
	}
	else if (shapeType.startsWith("MULTIPOLYGON"))
	{
		return QGis::WKBMultiPolygon;
	}
	else
	{
		return QGis::WKBUnknown;
	}
}

void QgsNGIDataSource::loadSchemas( QString ndaFilename )
{
	clearSchemas();

	
	QFile ndaFile(ndaFilename);

	if(ndaFile.exists() == false)
	{
		return;
	}

	if (!ndaFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

	QTextStream InStream(&ndaFile); 

	while(!InStream.atEnd())
	{
		QString line = InStream.readLine();
		if(line.compare(line, "$LAYER_NAME", Qt::CaseInsensitive) == 0)
		{
			
			QgsFeatureAttributeMap* fields = new QgsFeatureAttributeMap();
			QgsNgiFieldInfo*	fieldInfo = new QgsNgiFieldInfo();
			fieldInfo->fields = new QgsFields();
			fieldInfo->featureCount = 0;

			line = InStream.readLine();

			QString layername = line.mid(1, line.length()-2);
// 			qint64	ndapos = InStream.device()->pos();
	
			QgsNGILayer* layer = this->getLayerByName(layername);
// 			if(createSchema(&InStream, fieldInfo, fields) == true)
// 			{
// 				mLayerSchemas.insert(layername, fieldInfo);
// 				mLayerFields.insert(layername, fields);
// 			}
			if(createSchema(&InStream, layer) == true)
			{
				mLayerSchemas.insert(layername, fieldInfo);
				mLayerFields.insert(layername, fields);
			}
			else
			{
				delete fields;
				delete fieldInfo;
			}
		}
	}

	ndaFile.close();
}

size_t QgsNGIDataSource::getLayerCount()
{
	return mLayers.size();
}


QgsNgiFieldInfo* QgsNGIDataSource::getFieldInfo(QString layername)
{
	QgsDebugMsg("get feild info : " + layername);
	if(mLayerSchemas.find(layername) != mLayerSchemas.end())
	{
		
		return (QgsNgiFieldInfo*)mLayerSchemas.value(layername);
	}

	QgsDebugMsg("get feild info is null ");
	return NULL;
}


bool QgsNGIDataSource::createSchema( QTextStream* InStream, QgsNGILayer* layer)
{
	if(InStream == NULL || layer == NULL)
	{
		return false;
	}

	QString line, values, fieldname;
	QStringList attribs;
	bool initSchema = false;
	while(!InStream->atEnd())
	{
		line = InStream->readLine();

		if(line.startsWith("$ASPATIAL_FIELD_DEF", Qt::CaseInsensitive) == true)
		{
			QgsFields fields;
			while(!InStream->atEnd() && line.compare("<LAYER_END>", Qt::CaseInsensitive) != 0)
			{
				line = InStream->readLine();
				// 1. read field definition
				if(line.startsWith("ATTRIB", Qt::CaseInsensitive) == true)	// ATTRIB("ID",NUMERIC, 9, 0, TRUE)
				{
					values = line.mid(7, line.length()-8);
					QStringList attribs = values.split(",");
					if(attribs.length() == 5)
					{
						setFields(attribs, &fields);
					}
				}

				// 2. read record
				if(line.startsWith("$RECORD", Qt::CaseInsensitive) == true)	// $RECORD 1
				{
					if(initSchema == false)
					{
						layer->getLayerData()->setSchema(fields);
						initSchema = true;
					}

					line = InStream->readLine();	// 746, "", "", "일반주택", "주택", "", 1, "1000367100375B00110000000000007467"
					readRecordData(line, layer);
				}
			}

			return layer->getLayerData()->getSchema()->isEmpty();
		}
	}

	return false;

}
// 
// bool QgsNGIDataSource::createSchema( QTextStream* InStream, QgsNgiFieldInfo* finfo, QgsFeatureAttributeMap* fields)
// {
// 	if(InStream == NULL || finfo == NULL)
// 	{
// 		return false;
// 	}
// 
// 	QString line, values, fieldname;
// 	QStringList attribs;
// 	while(!InStream->atEnd())
// 	{
// 		line = InStream->readLine();
// 
// 		if(line.startsWith("$ASPATIAL_FIELD_DEF", Qt::CaseInsensitive) == true)
// 		{
// 			while(!InStream->atEnd() && line.compare("<LAYER_END>", Qt::CaseInsensitive) != 0)
// 			{
// 				line = InStream->readLine();
// 				// 1. read field definition
// 				if(line.startsWith("ATTRIB", Qt::CaseInsensitive) == true)	// ATTRIB("ID",NUMERIC, 9, 0, TRUE)
// 				{
// 					values = line.mid(7, line.length()-8);
// 					QStringList attribs = values.split(",");
// 					if(attribs.length() == 5)
// 					{
// 						setFields(attribs, finfo);
// 					}
// 				}
// 
// 				// 2. read record
// 				if(line.startsWith("$RECORD", Qt::CaseInsensitive) == true)	// $RECORD 1
// 				{
// 					line = InStream->readLine();	// 746, "", "", "일반주택", "주택", "", 1, "1000367100375B00110000000000007467"
// 					readRecordData(line, finfo, fields);
// 					finfo->featureCount++;
// 				}
// 			}
// 			
// 			return !finfo->fields->isEmpty();
// 		}
// 	}
// 
// 	return false;
// 
// }


void QgsNGIDataSource::setFields( QStringList& list, QgsFields* fields)
{
	if(fields == NULL)	return;

	QString typeName, name;
	int decimal;

	name = list.at(0);
	name.remove('"');
	typeName = list.at(1);

	if(typeName.compare("NUMERIC", Qt::CaseInsensitive) == 0)
	{
		decimal = list.at(3).toInt();
		if(decimal == 0)
			fields->append(QgsField(name,QVariant::Int));
		else
			fields->append(QgsField(name,QVariant::Double));
	} 
	else if(typeName.compare("DATE", Qt::CaseInsensitive) == 0)
	{
		fields->append(QgsField(name, QVariant::String));
	}
	else if(typeName.compare("STRING", Qt::CaseInsensitive) == 0)
	{
		int length = list.at(2).toInt();
		
		fields->append(QgsField(name, QVariant::String, QVariant::typeToName(QVariant::String), length));
	}
}

void QgsNGIDataSource::setFields( QStringList& list, QgsNgiFieldInfo* finfo)
{
	if(finfo == NULL || finfo->fields == NULL)	return;

	QString typeName, name;
	int decimal;

	name = list.at(0);
	name.remove('"');
	typeName = list.at(1);

	if(typeName.compare("NUMERIC", Qt::CaseInsensitive) == 0)
	{
		decimal = list.at(3).toInt();
		if(decimal == 0)
			finfo->fields->append(QgsField(name,QVariant::Int));
		else
			finfo->fields->append(QgsField(name,QVariant::Double));
	} 
	else if(typeName.compare("DATE", Qt::CaseInsensitive) == 0)
	{
		finfo->fields->append(QgsField(name, QVariant::String));
	}
	else if(typeName.compare("STRING", Qt::CaseInsensitive) == 0)
	{
		int length = list.at(2).toInt();
		
		finfo->fields->append(QgsField(name, QVariant::String, QVariant::typeToName(QVariant::String), length));
	}
}

QgsNGILayer* QgsNGIDataSource::getLayerData( int idx )
{
	throw std::exception("The method or operation is not implemented.");
}

QgsNGILayer* QgsNGIDataSource::getLayerByIndex( int index )
{
	QMap<QString, QgsNGILayer*>::ConstIterator ii = mLayers.begin();
	for(ii; ii!= mLayers.end(); ii++)
	{
		QgsNGILayer* layer = ii.value();
		QgsDebugMsg( "layername : " + layer->getName() + " , index : " + QString::number(layer->getIndex()));
		if(layer->getIndex() == index)
		{
			return layer;
		}
	}

	return NULL;
}

QgsNGILayer* QgsNGIDataSource::getLayer( int index )
{
	if(index < 0 || index >= mLayers.size())	
	{
		QgsDebugMsg( "layer index is invalid. request index is: " + QString::number(index));
		return NULL;
	}

	int count = 0;
	QMap<QString, QgsNGILayer*>::ConstIterator ii = mLayers.begin();
	for(ii; ii!= mLayers.end(); ii++, count++)
	{
		if(count == index)
		{
			QgsNGILayer* layer = ii.value();
			QgsDebugMsg( "layername : " + layer->getName() + " , index : " + QString::number(layer->getIndex()));

			return layer;
		}
	}


	return NULL;
}



// void QgsNGIDataSource::readRecordData(QString& line, QgsNgiFieldInfo* finfo, QgsFeatureAttributeMap* fields)
void QgsNGIDataSource::readRecordData(QString& line, QgsNGILayer* layer)
{
	QgsNGILayerData* layerData = layer->getLayerData();
	
	QgsFields* fieldSchemas = layerData->getSchema();
	int counts = fieldSchemas->count();

	QStringList attribs = line.split(", ");
	if(attribs.length() == counts)
	{
//		pushError( tr( "field definition count(%1) and field count(%2) not match ).arg( counts ).arg( attribs.length() ) );
	}

	QgsAttributes* attributes = new QgsAttributes;
	int		 geometryID;
	for(int i=0; i<counts; i++)
	{
		QVariant value;
		
		QString svalue = attribs.at(i);
		switch((*fieldSchemas)[i].type())
		{
		case QVariant::String:
			{
				if(svalue.length() == 2)
				{
					value = QString();
				}
				else
				{	
					svalue.remove(QChar('"'), Qt::CaseInsensitive);
					value = svalue;
				}
			}
			break;

		case QVariant::Int:
			{
				value = svalue.toInt();
				
			}
			break;

		case QVariant::Double:
			{
				value = svalue.toDouble();
			}
			break;

		default:
			break;
		}

		if(i==0)
		{
			geometryID = value.toInt();
		}

// 		attributes->append(value);
 		*attributes << value;
	}

	layerData->appendAttribute(geometryID, attributes);
}

// void QgsNGIDataSource::readRecordData(QString& line, QgsNgiFieldInfo* finfo, QgsFeatureAttributeMap* fields)
// {
// 	QgsFields* fieldSchemas = finfo->fields;
// 	int counts = fieldSchemas->count();
// 
// 	QStringList attribs = line.split(", ");
// 	if(attribs.length() == counts)
// 	{
// //		pushError( tr( "field definition count(%1) and field count(%2) not match ).arg( counts ).arg( attribs.length() ) );
// 	}
// 
// 	QgsAttributes attributes;
// 	int		 geometryID;
// 	for(int i=0; i<counts; i++)
// 	{
// 		QVariant value;
// 		
// 		QString svalue = attribs.at(i);
// 		switch((*fieldSchemas)[i].type())
// 		{
// 		case QVariant::String:
// 			{
// 				if(svalue.length() == 2)
// 				{
// 					value = QString();
// 				}
// 				else
// 				{	
// 					svalue.remove(QChar('"'), Qt::CaseInsensitive);
// 					value = svalue;
// 				}
// 			}
// 			break;
// 
// 		case QVariant::Int:
// 			{
// 				value = svalue.toInt();
// 				
// 			}
// 			break;
// 
// 		case QVariant::Double:
// 			{
// 				value = svalue.toDouble();
// 			}
// 			break;
// 
// 		default:
// 			break;
// 		}
// 
// 		if(i==0)
// 		{
// 			geometryID = value.toInt();
// 		}
// 
// // 		QString test8 = value.toString();
// // 		QgsDebugMsg(test8);
// 		attributes << value;
// 	}
// 
// 	fields->insert(geometryID, attributes);
// }

