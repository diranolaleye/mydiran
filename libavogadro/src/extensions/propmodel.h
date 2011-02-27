/**********************************************************************
  propmodel.h - Models to hold properties

  Copyright (C) 2007 by Tim Vandermeersch

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef PROPMODEL_H
#define PROPMODEL_H


#include <openbabel/mol.h>

#include <avogadro/glwidget.h>
#include <avogadro/extension.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QTableView>
#include <QItemSelectionModel>

#ifndef BUFF_SIZE
#define BUFF_SIZE 256
#endif

namespace Avogadro {

 class PropertiesModel : public QAbstractTableModel
  {
    Q_OBJECT

     public slots:
       void updateTable();
       //       void primitiveAdded(Primitive *primitive);
       //       void primitiveRemoved(Primitive *primitive);
       void atomAdded(Atom *atom);
       void atomRemoved(Atom *atom);
       void bondAdded(Bond *bond);
       void bondRemoved(Bond *bond);
       void moleculeChanged();

     public:
       enum Type {
         OtherType=0,
         AtomType,
         BondType,
         AngleType,
         TorsionType,
         CartesianType,
         ConformerType,
         MoleculeType
       };

       explicit PropertiesModel(Type type, QObject *parent = 0);

       int rowCount(const QModelIndex &parent = QModelIndex()) const;
       int columnCount(const QModelIndex &parent = QModelIndex()) const;
       QVariant data(const QModelIndex &index, int role) const;
       Qt::ItemFlags flags(const QModelIndex &index) const;
       bool setData(const QModelIndex &index, const QVariant &value,
           int role = Qt::EditRole);
       QVariant headerData(int section, Qt::Orientation orientation,
           int role = Qt::DisplayRole) const;

       void setMolecule (Molecule *molecule);
       void cacheOBMol() const;

     private:
       int m_type;
       mutable int m_rowCount;
       Molecule *m_molecule;

       mutable bool m_validCache;
       mutable OpenBabel::OBMol *m_cachedOBMol;
 };

} // end namespace Avogadro

#endif
