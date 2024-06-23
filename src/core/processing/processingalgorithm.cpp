/***************************************************************************
  processingalgorithm.cpp - ProcessingAlgorithm

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


#include "processingalgorithm.h"

#include <qgsapplication.h>
#include <qgsprocessingalgorithm.h>
#include <qgsprocessingprovider.h>
#include <qgsprocessingregistry.h>
#include <qgsvectorlayer.h>
#include <qgsvectorlayerutils.h>


ProcessingAlgorithm::ProcessingAlgorithm( QObject *parent )
  : QObject( parent )
{
}

void ProcessingAlgorithm::setId( const QString &id )
{
  if ( mAlgorithmId == id )
  {
    return;
  }

  mAlgorithmId = id;
  mAlgorithm = !mAlgorithmId.isEmpty() ? QgsApplication::instance()->processingRegistry()->algorithmById( mAlgorithmId ) : nullptr;

  if ( mAlgorithmParametersModel )
  {
    mAlgorithmParametersModel->setAlgorithmId( id );
  }

  emit idChanged( mAlgorithmId );
}

QString ProcessingAlgorithm::displayName() const
{
  return mAlgorithm ? mAlgorithm->displayName() : QString();
}

QString ProcessingAlgorithm::shortHelp() const
{
  return mAlgorithm ? mAlgorithm->shortHelpString() : QString();
}

void ProcessingAlgorithm::setInPlaceLayer( QgsVectorLayer *layer )
{
  if ( mInPlaceLayer.data() == layer )
  {
    return;
  }

  mInPlaceLayer = layer;
  emit inPlaceLayerChanged();
}

void ProcessingAlgorithm::setInPlaceFeatures( const QList<QgsFeature> &features )
{
  if ( mInPlaceFeatures == features )
  {
    return;
  }

  mInPlaceFeatures = features;

  emit inPlaceFeaturesChanged();
}

void ProcessingAlgorithm::setParametersModel( ProcessingAlgorithmParametersModel *parametersModel )
{
  if ( mAlgorithmParametersModel == parametersModel )
  {
    return;
  }

  if ( mAlgorithmParametersModel )
  {
    disconnect( mAlgorithmParametersModel, &ProcessingAlgorithmParametersModel::algorithmIdChanged, this, &ProcessingAlgorithm::setId );
  }

  mAlgorithmParametersModel = parametersModel;

  if ( mAlgorithmParametersModel )
  {
    mAlgorithmParametersModel->setAlgorithmId( mAlgorithmId );
    connect( mAlgorithmParametersModel, &ProcessingAlgorithmParametersModel::algorithmIdChanged, this, &ProcessingAlgorithm::setId );
  }

  emit parametersModelChanged();
}

bool ProcessingAlgorithm::run()
{
  if ( !mAlgorithm || !mAlgorithmParametersModel )
  {
    return false;
  }

  QgsProcessingContext context;
  context.setFlags( QgsProcessingContext::Flags() );
  context.setProject( QgsProject::instance() );

  QgsProcessingFeedback feedback;
  context.setFeedback( &feedback );

  QVariantMap parameters = mAlgorithmParametersModel->toVariantMap();

  if ( mInPlaceLayer )
  {
    if ( mInPlaceFeatures.isEmpty() || !mAlgorithm->supportInPlaceEdit( mInPlaceLayer.data() ) )
    {
      return false;
    }

    context.expressionContext().appendScope( mInPlaceLayer->createExpressionContextScope() );

    QStringList featureIds;
    for ( const QgsFeature &feature : mInPlaceFeatures )
    {
      featureIds << QString::number( feature.id() );
    }

    QString inputParameterName = QStringLiteral( "INPUT" );
    //TODO: Fix QGIS overprotective API
    //const QgsProcessingFeatureBasedAlgorithm *featureBasedAlgorithm = dynamic_cast<const QgsProcessingFeatureBasedAlgorithm *>( mAlgorithm );
    //if ( featureBasedAlgorithm )
    //{
    //  parameters[featureBasedAlgorithm->inputParameterName()]
    //}
    parameters[inputParameterName] = QgsProcessingFeatureSourceDefinition( mInPlaceLayer->id(),
                                                                           false,
                                                                           -1,
                                                                           Qgis::ProcessingFeatureSourceDefinitionFlags(),
                                                                           Qgis::InvalidGeometryCheck(),
                                                                           QStringLiteral( "$id IN (%1)" ).arg( featureIds.join( ',' ) ) );
    parameters[QStringLiteral( "OUTPUT" )] = QStringLiteral( "memory:" );

    const QgsProcessingFeatureBasedAlgorithm *featureBasedAlgorithm = dynamic_cast<const QgsProcessingFeatureBasedAlgorithm *>( mAlgorithm );
    if ( featureBasedAlgorithm )
    {
      qDebug() << "featureBasedAlgorithm";
      mInPlaceLayer->startEditing();

      QgsProcessingFeatureBasedAlgorithm *alg = static_cast<QgsProcessingFeatureBasedAlgorithm *>( featureBasedAlgorithm->create( { { QStringLiteral( "IN_PLACE" ), true } } ) );
      if ( !alg->prepare( parameters, context, &feedback ) )
      {
        return false;
      }

      QgsFeatureList features = mInPlaceFeatures;
      for ( const QgsFeature &feature : features )
      {
        QgsFeature inputFeature = QgsFeature( feature );
        context.expressionContext().setFeature( inputFeature );
        QgsFeatureList outputFeatures = alg->processFeature( inputFeature, context, &feedback );
        outputFeatures = QgsVectorLayerUtils::makeFeaturesCompatible( outputFeatures, mInPlaceLayer.data() );

        if ( outputFeatures.isEmpty() )
        {
          // Algorithm deleted the feature, remove from the layer
          mInPlaceLayer->deleteFeature( feature.id() );
        }
        else if ( outputFeatures.size() == 1 )
        {
          // Algorithm modified the feature, adjust accordingly
          QgsGeometry outputGeometry = outputFeatures[0].geometry();
          if ( !outputGeometry.equals( feature.geometry() ) )
          {
            mInPlaceLayer->changeGeometry( feature.id(), outputGeometry );
          }
          if ( outputFeatures[0].attributes() != feature.attributes() )
          {
            qDebug() << "attribute(s) changed!";
            QgsAttributeMap newAttributes;
            QgsAttributeMap oldAttributes;
            const QgsFields fields = mInPlaceLayer->fields();
            for ( const QgsField &field : fields )
            {
              const int index = fields.indexOf( field.name() );
              if ( outputFeatures[0].attribute( index ) != feature.attribute( index ) )
              {
                newAttributes[index] = outputFeatures[0].attribute( index );
                oldAttributes[index] = feature.attribute( index );
              }
            }
            mInPlaceLayer->changeAttributeValues( feature.id(), newAttributes, oldAttributes );
          }
        }
        else if ( outputFeatures.size() > 1 )
        {
          qDebug() << "got more than 1!";
          QgsFeatureList newFeatures;
          for ( QgsFeature &outputFeature : outputFeatures )
          {
            newFeatures << QgsVectorLayerUtils::createFeature( mInPlaceLayer.data(), outputFeature.geometry(), outputFeature.attributes().toMap(), &context.expressionContext() );
          }
          mInPlaceLayer->addFeatures( newFeatures );
        }
      }

      mInPlaceLayer->commitChanges();
    }
    {
      // Currently, only feature-based algorithms are supported
      return false;
    }
  }
  else
  {
    // Currently, only in-place algorithms are supported
    return false;
  }
}
