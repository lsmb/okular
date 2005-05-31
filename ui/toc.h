/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_TOC_H_
#define _KPDF_TOC_H_

#include <qdom.h>
#include <klistview.h>
#include "core/observer.h"

class KPDFDocument;

class TOC : public KListView, public DocumentObserver
{
Q_OBJECT
    public:
        TOC(QWidget *parent, KPDFDocument *document);

        // inherited from DocumentObserver
        uint observerId() const;
        void notifySetup( const QVector< KPDFPage * > & pages, bool documentChanged );

    signals:
        void hasTOC(bool has);

    private slots:
        void slotExecuted(Q3ListViewItem *i);

    private:
        void addChildren( const QDomNode & parentNode, KListViewItem * parentItem = 0 );
        KPDFDocument *m_document;
};

#endif
