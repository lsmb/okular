/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificateviewer.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <KColumnResizer>

#include <QLabel>
#include <QTextEdit>
#include <QTreeView>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QCryptographicHash>

#include "signatureguiutils.h"

CertificateModel::CertificateModel( Okular::CertificateInfo *certInfo, QObject * parent )
  : QAbstractTableModel( parent ), m_certificateInfo( certInfo )
{
    m_certificateProperties.append( qMakePair( i18n("Version"), i18n("V%1", QString::number( m_certificateInfo->version() )) ) );
    m_certificateProperties.append( qMakePair( i18n("Serial Number"), m_certificateInfo->serialNumber().toHex(' ') ) );
    m_certificateProperties.append( qMakePair( i18n("Issuer"), m_certificateInfo->issuerInfo(Okular::CertificateInfo::DistinguishedName) ) );
    m_certificateProperties.append( qMakePair( i18n("Issued On"), m_certificateInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_certificateProperties.append( qMakePair( i18n("Expires On"), m_certificateInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_certificateProperties.append( qMakePair( i18n("Subject"), m_certificateInfo->subjectInfo(Okular::CertificateInfo::DistinguishedName) ) );
    m_certificateProperties.append( qMakePair( i18n("Public Key"), i18n("%1 (%2 bits)", SignatureGuiUtils::getReadablePublicKeyType( m_certificateInfo->publicKeyType() ),
                                                                        m_certificateInfo->publicKeyStrength()) ) );
    m_certificateProperties.append( qMakePair( i18n("Key Usage"), SignatureGuiUtils::getReadableKeyUsage( m_certificateInfo->keyUsageExtensions() ) ) );
}


int CertificateModel::columnCount( const QModelIndex & ) const
{
    return 2;
}

int CertificateModel::rowCount( const QModelIndex & ) const
{
    return m_certificateProperties.size();
}

QVariant CertificateModel::data( const QModelIndex &index, int role ) const
{
    int row = index.row();
    if ( !index.isValid() || row < 0 || row >= m_certificateProperties.count() )
        return QVariant();

    switch ( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        switch ( index.column() )
        {
            case 0:
                return m_certificateProperties[row].first;
            case 1:
                return m_certificateProperties[row].second;
            default:
                return QString();
        }
        case PropertyKeyRole:
            return m_certificateProperties[row].first;
        case PropertyValueRole:
            return m_certificateProperties[row].second;
        case PublicKeyRole:
            return QString( m_certificateInfo->publicKey().toHex(' ') );
    }

    return QVariant();
}

QVariant CertificateModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role == Qt::TextAlignmentRole )
        return QVariant( Qt::AlignLeft );

    if ( orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch ( section )
    {
        case 0:
            return i18n("Property");
        case 1:
            return i18n("Value");
        default:
            return QVariant();
    }
}

CertificateViewer::CertificateViewer( Okular::CertificateInfo *certInfo, QWidget *parent )
    : KPageDialog( parent ), m_certificateInfo( certInfo )
{
    setModal( true );
    setMinimumSize( QSize( 500, 500 ));
    setFaceType( Tabbed );
    setWindowTitle( i18n("Certificate Viewer") );
    setStandardButtons( QDialogButtonBox::Close );

    auto exportBtn = new QPushButton( i18n("Export...") );
    connect( exportBtn, &QPushButton::clicked, this, &CertificateViewer::exportCertificate );
    addActionButton( exportBtn );

    // General tab
    auto generalPage = new QFrame( this );
    addPage( generalPage, i18n("General") );

    auto issuerBox = new QGroupBox( i18n("Issued By"), generalPage );
    auto issuerFormLayout = new QFormLayout( issuerBox );
    issuerFormLayout->setLabelAlignment( Qt::AlignLeft );
    issuerFormLayout->addRow( i18n("Common Name(CN)"), new QLabel( m_certificateInfo->issuerInfo( Okular::CertificateInfo::CommonName ) ) );
    issuerFormLayout->addRow( i18n("EMail"), new QLabel( m_certificateInfo->issuerInfo( Okular::CertificateInfo::EmailAddress ) ) );
    issuerFormLayout->addRow( i18n("Organization(O)"), new QLabel( m_certificateInfo->issuerInfo( Okular::CertificateInfo::Organization ) ) );

    auto subjectBox = new QGroupBox( i18n("Issued To"), generalPage );
    auto subjectFormLayout = new QFormLayout( subjectBox );
    subjectFormLayout->setLabelAlignment( Qt::AlignLeft );
    subjectFormLayout->addRow( i18n("Common Name(CN)"), new QLabel( m_certificateInfo->subjectInfo( Okular::CertificateInfo::CommonName ) ) );
    subjectFormLayout->addRow( i18n("EMail"), new QLabel( m_certificateInfo->subjectInfo( Okular::CertificateInfo::EmailAddress ) ) );
    subjectFormLayout->addRow( i18n("Organization(O)"), new QLabel( m_certificateInfo->subjectInfo( Okular::CertificateInfo::Organization ) ) );

    auto validityBox = new QGroupBox( i18n("Validity"), generalPage );
    auto validityFormLayout = new QFormLayout( validityBox );
    validityFormLayout->setLabelAlignment( Qt::AlignLeft );
    validityFormLayout->addRow( i18n("Issued On"), new QLabel( m_certificateInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    validityFormLayout->addRow( i18n("Expires On"), new QLabel( m_certificateInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );

    auto fingerprintBox = new QGroupBox( i18n("Fingerprints"), generalPage );
    auto fingerprintFormLayout = new QFormLayout( fingerprintBox );
    fingerprintFormLayout->setLabelAlignment( Qt::AlignLeft );
    QByteArray certData = m_certificateInfo->certificateData();
    auto sha1Label = new QLabel( QString( QCryptographicHash::hash( certData, QCryptographicHash::Sha1 ).toHex(' ') ) );
    sha1Label->setWordWrap( true );
    auto sha256Label = new QLabel( QString( QCryptographicHash::hash( certData, QCryptographicHash::Sha256 ).toHex(' ') ) );
    sha256Label->setWordWrap( true );
    fingerprintFormLayout->addRow( i18n("SHA-1 Fingerprint"), sha1Label );
    fingerprintFormLayout->addRow( i18n("SHA-256 Fingerprint"), sha256Label );

    auto generalPageLayout = new QVBoxLayout( generalPage );
    generalPageLayout->addWidget( issuerBox );
    generalPageLayout->addWidget( subjectBox );
    generalPageLayout->addWidget( validityBox );
    generalPageLayout->addWidget( fingerprintBox );

    //force column 1 to have same width
    auto resizer = new KColumnResizer( this );
    resizer->addWidgetsFromLayout( issuerBox->layout(), 0 );
    resizer->addWidgetsFromLayout( subjectBox->layout(), 0 );
    resizer->addWidgetsFromLayout( validityBox->layout(), 0 );
    resizer->addWidgetsFromLayout( fingerprintBox->layout(), 0 );

    // Details tab
    auto detailsFrame = new QFrame( this );
    addPage( detailsFrame, i18n("Details") );
    auto certDataLabel = new QLabel( i18n("Certificate Data:") );
    auto certTree = new QTreeView( this );
    certTree->setIndentation( 0 );
    m_certificateModel = new CertificateModel( m_certificateInfo, this );
    certTree->setModel( m_certificateModel );
    //QItemSelectionModel::currentChanged because QTreeView::activated only works for mouse.
    connect( certTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CertificateViewer::updateText );
    m_propertyText = new QTextEdit( this );
    m_propertyText->setReadOnly( true );

    auto detailsPageLayout = new QVBoxLayout( detailsFrame );
    detailsPageLayout->addWidget( certDataLabel );
    detailsPageLayout->addWidget( certTree );
    detailsPageLayout->addWidget( m_propertyText );
}

void CertificateViewer::updateText( const QModelIndex &index )
{
    QString key = m_certificateModel->data( index, CertificateModel::PropertyKeyRole ).toString();
    if ( key == QLatin1String("Public Key") )
        m_propertyText->setText( m_certificateModel->data( index, CertificateModel::PublicKeyRole ).toString() );
    else
    {
        const QString propertyValue = m_certificateModel->data( index, CertificateModel::PropertyValueRole ).toString();
        QString textToView;
        foreach ( auto c, propertyValue )
        {
            if ( c == ',' )
                textToView += '\n';
            else
                textToView += c;
        }
        m_propertyText->setText( textToView );
    }
}

void CertificateViewer::exportCertificate()
{
    const QString caption = i18n("Where do you want to save this certificate?");
    const QString path = QFileDialog::getSaveFileName( this, caption, QStringLiteral("Certificate.cer"), i18n("Certificate File (*.cer)") );
    if ( !path.isEmpty() )
    {
        QFile targetFile( path );
        targetFile.open( QIODevice::WriteOnly );
        if ( targetFile.write( m_certificateInfo->certificateData() ) == -1 )
        {
            KMessageBox::error( this, i18n("Unable to export certificate!") );
        }
        targetFile.close();
    }
}
