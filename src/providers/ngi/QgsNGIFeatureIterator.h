#pragma once
#include "qgsfeatureiterator.h"

#include <QList>


class	QgsGeometry;


class	QgsNGIProvider;
class   QgsNGIDataSource;
class   QgsNGILayer;
class	QgsNGILayerData;

class QgsNGIFeatureSource : public QgsAbstractFeatureSource
{
  public:
    QgsNGIFeatureSource( QgsNGIProvider* p );
    ~QgsNGIFeatureSource();

    virtual QgsFeatureIterator getFeatures( const QgsFeatureRequest& request ) override;
	
	QgsNGIProvider* getProvider() { return mProvider; }

  protected:

	QgsNGIProvider* mProvider;

    friend class QgsNGIFeatureIterator;
};



class QgsNGIFeatureIterator :	public QgsAbstractFeatureIteratorFromSource<QgsNGIFeatureSource>
{
public:
	typedef QMap<QgsFeatureId, QgsFeature*> QgsFeaturesMap;

	QgsNGIFeatureIterator(QgsNGIFeatureSource* source, bool ownSource, const QgsFeatureRequest& request );
	virtual ~QgsNGIFeatureIterator(void);

    //! reset the iterator to the starting position
    virtual bool rewind();

    //! end of iterating: free the resources / lock
    virtual bool close();

  protected:
    //! fetch next feature, return true on success
    virtual bool fetchFeature( QgsFeature& feature );

	bool setSubsetString( QString& subsetString );
	bool filterEval( QgsFeature& feature );

	bool nextFeatureUsingList( QgsFeature& feature );
	bool nextFeatureTraverseAll( QgsFeature& feature );

	void setFeatureAttributes(QgsFeature& feature);

//     bool readFeature( OGRFeatureH fet, QgsFeature& feature );
// 
//     //! Get an attribute associated with a feature
//     void getFeatureAttribute( OGRFeatureH ogrFet, QgsFeature & f, int attindex );


protected:

	QList<QgsFeatureId> mFeatureIdList;
	QList<QgsFeatureId>::iterator mFeatureIdListIterator;

	QgsNGILayerData*	mLayerData;

	QgsGeometry*		mSelectRectGeom;
	QgsFeaturesMap::iterator mSelectIterator;
	QgsExpression*		mExp;

	QgsNGIProvider*		mProvider;
    
	QgsNGIDataSource*	mDataSource;
	
	QgsNGILayer*		mLayer;

	bool				mFeatureFetched;

    bool				mSubsetStringSet;
	bool mUsingFeatureIdList;

};
