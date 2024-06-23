/***************************************************************************
  processingalgorithm.h - ProcessingAlgorithm

 ---------------------
 begin                : 22.06.2024
 copyright            : (C) 2024 by Mathieu Pellerin
 email                : mathieu at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef PROCESSINGALGORITHM
#define PROCESSINGALGORITHM

#include "processingalgorithmparametersmodel.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <qgsfeature.h>

class QgsProcessingProvider;
class QgsProcessingAlgorithm;
class QgsVectorLayer;

/**
 * \brief A processing algorithm item capable of runnning a given algorithm.
 */
class ProcessingAlgorithm : public QObject
{
    Q_OBJECT

    Q_PROPERTY( QString id READ id WRITE setId NOTIFY idChanged )
    Q_PROPERTY( bool isValid READ isValid NOTIFY idChanged )

    Q_PROPERTY( QString displayName READ displayName NOTIFY idChanged )
    Q_PROPERTY( QString shortHelp READ shortHelp NOTIFY idChanged )
    Q_PROPERTY( ProcessingAlgorithmParametersModel *parametersModel READ parametersModel WRITE setParametersModel NOTIFY parametersModelChanged )

    Q_PROPERTY( QgsVectorLayer *inPlaceLayer READ inPlaceLayer WRITE setInPlaceLayer NOTIFY inPlaceLayerChanged )
    Q_PROPERTY( QList<QgsFeature> inPlaceFeatures READ inPlaceFeatures WRITE setInPlaceFeatures NOTIFY inPlaceFeaturesChanged )

  public:
    explicit ProcessingAlgorithm( QObject *parent = nullptr );

    /**
     * Returns the current algorithm ID from which parameters are taken from.
     */
    QString id() const { return mAlgorithmId; }

    /**
     * Sets the current algorithm \a ID from which parameters are taken from.
     */
    void setId( const QString &id );

    /**
     * Returns whether the current model refers to a valid algorithm.
     */
    bool isValid() const { return mAlgorithm; }

    /**
     * Returns the display name of the algorithm.
     */
    QString displayName() const;

    /**
     * Returns a short description of the algorithm.
     */
    QString shortHelp() const;

    /**
     * Returns the vector \a layer for in-place algorithm filter.
     */
    QgsVectorLayer *inPlaceLayer() const { return mInPlaceLayer.data(); }

    /**
     * Sets the vector \a layer for in-place algorithm filter.
     */
    void setInPlaceLayer( QgsVectorLayer *layer );

    /**
     * Returns the vector \a layer for in-place algorithm filter.
     */
    QList<QgsFeature> inPlaceFeatures() const { return mInPlaceFeatures; }

    /**
     * Sets the vector \a layer for in-place algorithm filter.
     */
    void setInPlaceFeatures( const QList<QgsFeature> &features );

    /**
     * Returns the algorithm parameters model.
     */
    ProcessingAlgorithmParametersModel *parametersModel() const { return mAlgorithmParametersModel; }

    /**
     * Sets the algorithm parameters model.
     */
    void setParametersModel( ProcessingAlgorithmParametersModel *parametersModel );

    /**
     * Executes the algorithm.
     */
    Q_INVOKABLE bool run();

  signals:
    /**
     * Emitted when the algorithm ID has changed
     */
    void idChanged( const QString &id );

    /**
    * Emitted when the parameter model has changed
    */
    void parametersModelChanged();

    /**
     * Emitted when the in place vector layer has changed
     */
    void inPlaceLayerChanged();

    /**
     * Emitted when the in place feature IDs list has changed
     */
    void inPlaceFeaturesChanged();

  private:
    QString mAlgorithmId;
    const QgsProcessingAlgorithm *mAlgorithm = nullptr;
    ProcessingAlgorithmParametersModel *mAlgorithmParametersModel = nullptr;

    QPointer<QgsVectorLayer> mInPlaceLayer;
    QList<QgsFeature> mInPlaceFeatures;
};

#endif // PROCESSINGALGORITHM
