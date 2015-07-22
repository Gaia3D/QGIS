#pragma once
#include "qgsvectordataprovider.h"

#include <QString>
#include "QTextCodec"



class QgsField;
class QgsSpatialIndex;

class QgsNGIDataSource;
class QgsNGILayer;

class QgsNGIFeatureIterator;



#define TO8(x)   (x).toUtf8().constData()
#define TO8F(x)  (x).toUtf8().constData()
#define FROM8(x) QString::fromUtf8(x)



class QgsNGIProvider : public QgsVectorDataProvider
{
    Q_OBJECT

public:

	/**
     * Constructor of the vector provider
     * @param uri  uniform resource locator (URI) for a dataset
     */
	QgsNGIProvider(QString const & uri = "" );
	
	/**
     * Destructor
     */
    virtual ~QgsNGIProvider();

    /* Implementation of functions from QgsVectorDataProvider */

    virtual QgsAbstractFeatureSource* featureSource() const override;

    virtual QgsCoordinateReferenceSystem crs();

    /**
     *   Returns the permanent storage type for this layer as a friendly name.
     */
    virtual QString storageType() const override;
	
    /**
     * Sub-layers handled by this provider, in order from bottom to top
     *
     * Sub-layers are used when the provider's source can combine layers
     * it knows about in some way before it hands them off to the provider.
     */
    virtual QStringList subLayers() const;

	virtual void setEncoding( const QString& e );

	QgsNGIDataSource* open(QString& filePath);

	QgsNGIDataSource* getDataSource() { return mNgiSource;};

	/** return the number of layers for the current data source

    @note

	Should this be subLayerCount() instead?
    */
    virtual size_t layerCount() const;

	QString layerName() { return mLayerName; }

	virtual bool supportsSubsetString() { return true; }

	QString subsetString() { return mSubsetString; }

	virtual QGis::WkbType geometryType() const;

    /** Update the extents
     */
    virtual void updateExtents();



    /**
     * Get the number of features in the layer
     */
    virtual long featureCount() const;

    /**
     * Get the field information for the layer
     */
    virtual const QgsFields & fields() const;

	
	/** Return the extent for this data layer
     */
    virtual QgsRectangle extent();


    /**
     * Returns true if this is a valid layer. It is up to individual providers
     * to determine what constitutes a valid layer
     */
    virtual bool isValid();

    /** return a provider name

    Essentially just returns the provider key.  Should be used to build file
    dialogs so that providers can be shown with their supported types. Thus
    if more than one provider supports a given format, the user is able to
    select a specific provider to open that file.

    @note

    Instead of being pure virtual, might be better to generalize this
    behavior and presume that none of the sub-classes are going to do
    anything strange with regards to their name or description?

    */
    virtual QString name() const;


	

    /** return description

      Return a terse string describing what the provider is.

      @note

      Instead of being pure virtual, might be better to generalize this
      behavior and presume that none of the sub-classes are going to do
      anything strange with regards to their name or description?

     */
    virtual QString description() const;


    /** Returns a bitmask containing the supported capabilities
    Note, some capabilities may change depending on whether
    a spatial filter is active on this provider, so it may
    be prudent to check this value per intended operation.
     */
    virtual int capabilities() const;

	/**
     * Query the provider for features specified in request.
     */
    virtual QgsFeatureIterator getFeatures( const QgsFeatureRequest& request = QgsFeatureRequest() );


	/** return vector file filter string

      Returns a string suitable for a QFileDialog of vector file formats
      supported by the data provider.  Naturally this will be an empty string
      for those data providers that do not deal with plain files, such as
      databases and servers.

      @note

      It'd be nice to eventually be raster/vector neutral.
    */
    /* virtual */
    QString fileVectorFilters() const;
    /** return a string containing the available database drivers */
    QString databaseDrivers() const;
    /** return a string containing the available directory drivers */
    QString protocolDrivers() const;
    /** return a string containing the available protocol drivers */
    QString directoryDrivers() const;
	int layerIndex();

public:
	/** Flag whether OGR will return fields required by nextFeature() calls.
        The relevant fields are first set in select(), however the setting may be
        interferred by some other calls. This flag ensures they are set again
        to correct values.
     */
    bool mRelevantFieldsForNextFeature;


private:

	friend class QgsNGIFeatureIterator;

	//! path to filename
	QString mFilePath;

	//! layer name
	QString mLayerName;

	//! layer index
	int mLayerIndex;

	QgsFields mAttributeFields;

    QgsRectangle mExtentRect;

	int				mGeomType;

	bool			mValid;


	//! current spatial filter
	QgsRectangle mFetchRect;

	//! String used to define a subset of the layer
	QString mSubsetString;


	mutable QStringList mSubLayerList;

 
	QgsRectangle		mExtent;
	bool				mIsUpdated;

	QgsNGIDataSource*	mNgiSource;
	QgsNGILayer*		mNgiOrigLayer;
	QgsNGILayer*		mNgiLayer;

	// Friendly name of the NGI Driver that was actually used to open the layer
	QString				mDriverName;

	QgsSpatialIndex*	mSpatialIndex;

	friend class QgsNGIDataSource;
	friend class QgsNGIFeatureIterator;
	QSet< QgsNGIFeatureIterator *> mActiveIterators;
};
