/**********************************************************************
  propmodel.cpp - Models to hold properties

  Copyright (C) 2007 by Tim Vandermeersch
  Copyright (C) 2009 by Konstantin Tokarev 

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

#include "propmodel.h"
// for recursively setting bond lengths, angles, etc.
#include "../tools/skeletontree.h"

#include <avogadro/molecule.h>
#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <openbabel/mol.h>
#include <Eigen/Geometry>

#include <limits>

#include <QDebug>

namespace Avogadro {

  using std::numeric_limits;
  using std::vector;
  using std::pair;
  using OpenBabel::triple;
  using OpenBabel::OBMol;
  using OpenBabel::OBAngleData;
  using OpenBabel::OBTorsionData;
  using OpenBabel::OBTorsion;
  using OpenBabel::OBGenericDataType::AngleData;
  using OpenBabel::OBGenericDataType::TorsionData;
  using OpenBabel::OBAtom;

  QString groupIndexString (Atom *a)
  {
    unsigned int gi = a->groupIndex();
    if (gi != 0) {
      return QString(OpenBabel::etab.GetSymbol(a->atomicNumber())) + QString("%L1").arg(gi);
    } else {
      return QString(OpenBabel::etab.GetSymbol(a->atomicNumber()));
    }
  }

  inline QString bondTypeString (Atom *a, Atom *b, int order)
  {
    Q_UNUSED(order)
    /*QString bond;
    if (order == 2) {
      bond = "=";
    } else if (order == 3) {
      bond = "\x2261"; //2261";
    } else {
      bond = "-";
    }*/
    return QString(OpenBabel::etab.GetSymbol(a->atomicNumber())) + '-' +
      QString(OpenBabel::etab.GetSymbol(b->atomicNumber()));
  }

  inline QString angleTypeString (Atom *a, Atom *b, Atom *c)
  {
    return QString(OpenBabel::etab.GetSymbol(a->atomicNumber())) +
      QString(OpenBabel::etab.GetSymbol(b->atomicNumber())) +
      QString(OpenBabel::etab.GetSymbol(c->atomicNumber()));
  }
  
  inline QString angleTypeString (Atom *a, Atom *b, Atom *c, Atom *d)
  {
    return QString(OpenBabel::etab.GetSymbol(a->atomicNumber())) + 
      QString(OpenBabel::etab.GetSymbol(b->atomicNumber())) +
      QString(OpenBabel::etab.GetSymbol(c->atomicNumber())) +
      QString(OpenBabel::etab.GetSymbol(d->atomicNumber()));
  }
  
  PropertiesModel::PropertiesModel(Type type, QObject *parent)
    : QAbstractTableModel(parent), m_type(type), m_rowCount(0), m_molecule(0),
      m_validCache(false), m_cachedOBMol(0)
  {
  }

  int PropertiesModel::rowCount(const QModelIndex &parent) const
  {
    Q_UNUSED(parent);

    if (m_type == AtomType) {
      return m_molecule->numAtoms();
    }
    else if (m_type == BondType) {
      return m_molecule->numBonds();
    }
    /*else if (m_type == CartesianType) {
      return m_molecule->numAtoms();
    }*/
    else if (m_type == ConformerType) {
      return m_molecule->numConformers();
    }
    else if (m_type == AngleType) {
      if (!m_validCache)
        cacheOBMol();
      m_cachedOBMol->FindAngles();
      OBAngleData *ad = static_cast<OBAngleData *>(m_cachedOBMol->GetData(AngleData));
      return ad->GetSize();
    }
    else if (m_type == TorsionType) {
      if (!m_validCache)
        cacheOBMol();
      m_cachedOBMol->FindTorsions();
      OBTorsionData *td = static_cast<OBTorsionData *>(m_cachedOBMol->GetData(TorsionData));
      vector<OBTorsion> torsions = td->GetData();
      vector<triple<OBAtom*,OBAtom*,double> > torsionADs;
      vector<OBTorsion>::iterator i;

      int rowCount = 0;
      for (i = torsions.begin(); i != torsions.end(); ++i) {
        torsionADs = i->GetADs();
        rowCount += torsionADs.size();
      }
      return rowCount;
    }
    return 0;
  }

  int PropertiesModel::columnCount(const QModelIndex &parent) const
  {
    Q_UNUSED(parent);
    switch (m_type) {
    case AtomType:
      return 8; // element, type, valence, formal charge, partial charge, x, y, z
    case BondType:
      return 6;
    case AngleType:
      return 5;
    case TorsionType:
      return 6;
    /*case CartesianType:
      return 3;*/
    case ConformerType:
      return 1;
    }
    return 0;
  }

  QVariant PropertiesModel::data(const QModelIndex &index, int role) const
  {
    if (!index.isValid())
      return QVariant();

    // handle text alignments
    if (role == Qt::TextAlignmentRole) {
      /*if (m_type == CartesianType) {
        return Qt::AlignRight + Qt::AlignVCenter; // XYZ coordinates
      }
      else*/ if (m_type == ConformerType) {
        return Qt::AlignRight + Qt::AlignVCenter; // energies
      }
      else if (m_type == AtomType) {
        if (index.column() == 3)
          return Qt::AlignRight + Qt::AlignVCenter; // partial charge
        else
          return Qt::AlignHCenter + Qt::AlignVCenter;
      }
      else if (m_type == BondType) {
        if (index.column() == 5)
          return Qt::AlignRight + Qt::AlignVCenter; // bond length
        else
          return Qt::AlignHCenter + Qt::AlignVCenter;
      }
      else if (m_type == AngleType) {
        if (index.column() == 4)
          return Qt::AlignRight + Qt::AlignVCenter; // angle
        else
          return Qt::AlignHCenter + Qt::AlignVCenter;
      }
      else if (m_type == TorsionType) {
        if (index.column() == 5)
          return Qt::AlignRight + Qt::AlignVCenter; // dihedral angle
        else
          return Qt::AlignHCenter + Qt::AlignVCenter;
      }
    }

    if (role != Qt::UserRole && role != Qt::DisplayRole)
      return QVariant();

    bool sortRole = (role == Qt::UserRole); // from the proxy model to handle floating-point

    if (m_type == AtomType) {
      if (static_cast<unsigned int>(index.row()) >= m_molecule->numAtoms())
        return QVariant();

      Atom *atom = m_molecule->atom(index.row());
      QString format("%L1");
      
      switch (index.column()) {
      case 0: // atomic symbol
        return QString(OpenBabel::etab.GetSymbol(atom->atomicNumber()));
      case 1: // type
        {
          if (!m_validCache)
            cacheOBMol();
          OpenBabel::OBAtom *obatom = m_cachedOBMol->GetAtom(index.row() + 1);
          return obatom->GetType();
        }
      case 2: // valence
        return atom->valence();
      case 3: // formal charge
        return atom->formalCharge();
      case 4: // partial charge
        if (sortRole)
          return atom->partialCharge();
        else
          return format.arg(atom->partialCharge(), 0, 'f', 3);
      case 5:
        if (sortRole)
          return atom->pos()->x();
        else
          return format.arg(atom->pos()->x(), 0, 'f', 5);
      case 6:
        if (sortRole)
          return atom->pos()->y();
        else
          return format.arg(atom->pos()->y(), 0, 'f', 5);
      case 7:
        if (sortRole)
          return atom->pos()->z();
        else
          return format.arg(atom->pos()->z(), 0, 'f', 5);
      }
    }
    else if (m_type == BondType) {
      if (static_cast<unsigned int>(index.row()) >= m_molecule->numBonds())
        return QVariant();

      if (!m_validCache)
        cacheOBMol();
      OpenBabel::OBBond *bond = m_cachedOBMol->GetBond(index.row());
      if (sortRole && index.column() == 4)
        return bond->GetLength();
      
      switch (index.column()) {
      case 0: // type
        return bondTypeString(m_molecule->atom(bond->GetBeginAtomIdx()-1),
          m_molecule->atom(bond->GetEndAtomIdx()-1), bond->GetBondOrder());
      case 1: // atom 1
        return groupIndexString(m_molecule->atom(bond->GetBeginAtomIdx()-1));
      case 2: // atom 2
        return groupIndexString(m_molecule->atom(bond->GetEndAtomIdx()-1));
      case 3: // order
        return bond->GetBondOrder();
      case 4: // rotatable
        if (bond->IsRotor()) {
            return tr("Yes");
        } else {
            return tr("No");
        }
      case 5: // length
        QString format("%L1");
        return format.arg(bond->GetLength(), 0, 'f', 4);
      }
    }
    else if (m_type == AngleType) {
      if (!m_validCache)
        cacheOBMol();
      m_cachedOBMol->FindAngles();
      OBAngleData *ad = static_cast<OBAngleData *>(m_cachedOBMol->GetData(AngleData));
      vector<vector<unsigned int> > angles;
      ad->FillAngleArray(angles);

      if ((unsigned int) index.row() >= angles.size())
        return QVariant();

      double angle;
      switch (index.column()) {
      case 0: // type
        return angleTypeString(m_molecule->atom(angles[index.row()][1]),
          m_molecule->atom(angles[index.row()][0]),
          m_molecule->atom(angles[index.row()][2]));
      case 1: // start atom
        return groupIndexString(m_molecule->atom(angles[index.row()][1]));
      case 2: // vertex -- yes, angles are filled by Open Babel with the vertex first
        return groupIndexString(m_molecule->atom(angles[index.row()][0]));
      case 3: // end atom
        return groupIndexString(m_molecule->atom(angles[index.row()][2]));
      case 4:
        angle = m_cachedOBMol->GetAngle(m_cachedOBMol->GetAtom(angles[index.row()][1] + 1),
                                        m_cachedOBMol->GetAtom(angles[index.row()][0] + 1),
                                        m_cachedOBMol->GetAtom(angles[index.row()][2] + 1));
        if (numeric_limits<double>::has_infinity &&
            angle == numeric_limits<double>::infinity()) {
          angle = 0.0;
        }
        QString format("%L1");
        if (sortRole)
          return angle;
        else
          return format.arg(angle, 0, 'f', 4);
      }
    }
    else if (m_type == TorsionType) {
      if (!m_validCache)
        cacheOBMol();
      m_cachedOBMol->FindTorsions();
      OBTorsionData *td = static_cast<OBTorsionData *>(m_cachedOBMol->GetData(TorsionData));
      vector<OBTorsion> torsions = td->GetData();
      pair<OBAtom*,OBAtom*> torsionBC;
      vector<triple<OBAtom*,OBAtom*,double> > torsionADs;
      vector<OBTorsion>::iterator i;
      vector<triple<OBAtom*,OBAtom*,double> >::iterator j;

      int rowCount = 0;
      double dihedralAngle;
      for (i = torsions.begin(); i != torsions.end(); ++i) {
        torsionBC = i->GetBC();
        torsionADs = i->GetADs();
        for (j = torsionADs.begin(); j != torsionADs.end(); ++j) {
          if (rowCount == index.row()) {
            switch (index.column()) {
            case 0:
              return angleTypeString(m_molecule->atom(j->first->GetIdx()-1),
                m_molecule->atom(torsionBC.first->GetIdx()-1),
                m_molecule->atom(torsionBC.second->GetIdx()-1),
                m_molecule->atom(j->second->GetIdx()-1));
            case 1:
              return groupIndexString(m_molecule->atom(j->first->GetIdx()-1));
            case 2:
              return groupIndexString(m_molecule->atom(torsionBC.first->GetIdx()-1));
            case 3:
              return groupIndexString(m_molecule->atom(torsionBC.second->GetIdx()-1));
            case 4:
              return groupIndexString(m_molecule->atom(j->second->GetIdx()-1));
            case 5:
              dihedralAngle = m_cachedOBMol->GetTorsion(j->first,
                                                        torsionBC.first,
                                                        torsionBC.second,
                                                        j->second);
              if (numeric_limits<double>::has_infinity &&
                  dihedralAngle == numeric_limits<double>::infinity()) {
                dihedralAngle = 0.0;
              }
              QString format("%L1");
              if (sortRole)
                return dihedralAngle;
              else
                return format.arg(dihedralAngle, 0, 'f', 4);
            }
          }
          rowCount++;
        }
      }
    } /*else if (m_type == CartesianType) {
      if (static_cast<unsigned int>(index.row()) >= m_molecule->numAtoms())
        return QVariant();

      Atom *atom = m_molecule->atom(index.row());
      QString format("%L1");

      switch (index.column()) {
      case 0:
        if (sortRole)
          return atom->pos()->x();
        else
          return format.arg(atom->pos()->x(), 0, 'f', 5);
      case 1:
        if (sortRole)
          return atom->pos()->y();
        else
          return format.arg(atom->pos()->y(), 0, 'f', 5);
      case 2:
        if (sortRole)
          return atom->pos()->z();
        else
          return format.arg(atom->pos()->z(), 0, 'f', 5);
      }
    }*/ else if (m_type == ConformerType) {
      if (static_cast<unsigned int>(index.row()) >= m_molecule->numConformers())
        return QVariant();

      switch (index.column()) {
      case 0: // energy
        if ((unsigned int) index.row() >= m_molecule->energies().size())
          return QVariant();

        QString format("%L1");
        if (sortRole)
          return m_molecule->energies().at(index.row());
        else
          return format.arg(m_molecule->energies().at(index.row()), 0, 'f', 4);
      }
    }

    return QVariant();
  }

  QVariant PropertiesModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
    // handle text alignments
    if (role == Qt::TextAlignmentRole) {
      if (orientation == Qt::Vertical) {
        return Qt::AlignHCenter; // XYZ coordinates
      }
    }

    if (role != Qt::DisplayRole)
      return QVariant();

    if (m_type == AtomType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return tr("Element");
        case 1:
          return tr("Type");
        case 2:
          return tr("Valence");
        case 3:
          return QString(tr("Formal Charge")).replace(" ","\n");
        case 4:
          return QString(tr("Partial Charge")).replace(" ","\n");
        case 5:
          return trUtf8("X %1", "in Angstrom").arg("(\xC5)");
        case 6:
          return trUtf8("Y %1", "in Angstrom").arg("(\xC5)");
        case 7:
          return trUtf8("Z  %1", "in Angstrom").arg("(\xC5)");
        }
      } else
        return tr("Atom %1").arg(section + 1);
    } else if (m_type == BondType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return tr("Type");
        case 1:
          return tr("Start Atom");
        case 2:
          return tr("End Atom");
        case 3:
          return tr("Bond Order");
        case 4:
          return tr("Rotatable");
        case 5:
          return tr("Length %1", "in Angstrom").arg("(\xC5)");
        }
      } else
        // Bond ordering starts at 0
        return tr("Bond %1").arg(section + 1);
    } else if (m_type == AngleType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return tr("Type");
        case 1:
          return tr("Start Atom");
        case 2:
          return tr("Vertex");
        case 3:
          return tr("End Atom");
        case 4:
          return tr("Angle %1", "Degree symbol").arg("(\xB0)");
        }
      } else
        return tr("Angle %1").arg(section + 1);
    } else if (m_type == TorsionType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return tr("Type");
        case 1:
        case 2:
        case 3:
        case 4:
          return tr("Atom %1").arg(section);
        case 5:
          return trUtf8("Torsion %1", "Degree symbol").arg("(\xB0)");
        }
      } else
        return tr("Torsion %1").arg(section + 1);
    } /*else if (m_type == CartesianType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return trUtf8("X %1", "in Angstrom").arg("(\xC5)");
        case 1:
          return trUtf8("Y %1", "in Angstrom").arg("(\xC5)");
        case 2:
          return trUtf8("Z  %1", "in Angstrom").arg("(\xC5)");
        }
      } else
        return tr("Atom %1").arg(section + 1);
    }*/ else if (m_type == ConformerType) {
      if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
          return tr("Energy");
        }
      } else
        return tr("Conformer %1").arg(section + 1);
    }

    return QVariant();
  }

  Qt::ItemFlags PropertiesModel::flags(const QModelIndex &index) const
  {
    if (!index.isValid())
      return Qt::ItemIsEnabled;

    if (m_type == AtomType) {
      switch (index.column()) {
      case 0: // atomic number
      case 3: // formal charge
      case 4: // partial charge
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
      case 1: // type
      case 2: // valence
        return QAbstractItemModel::flags(index);
      }
    }
    else if (m_type == BondType) {
      switch (index.column()) {
      case 4: // bond length
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
      default:
        return QAbstractItemModel::flags(index);
      }
    }
    else if (m_type == AngleType) {
      switch (index.column()) {
      case 3: // angle
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
      default:
        return QAbstractItemModel::flags(index);
      }
    }
    else if (m_type == TorsionType) {
      switch (index.column()) {
      case 4: // dihedral
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
      default:
        return QAbstractItemModel::flags(index);
      }
    }
    /*else if (m_type == CartesianType) {
      return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    }*/
    else if (m_type == ConformerType) {
      return QAbstractItemModel::flags(index);
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
  }

  bool PropertiesModel::setData(const QModelIndex &index,
                                const QVariant &value,
                                int role)
  {
    if (!index.isValid())
      return false;

    if (role != Qt::EditRole)
      return false;

    // If an item is actually editable, we should invalidate the cached OBMol
    // We can still use the cached molecule -- we just invalidate now
    // So that we can call "return" and have the cache invalid when we leave
    m_validCache = false;

    if (m_type == AtomType) {
      Atom *atom = m_molecule->atom(index.row());
      Eigen::Vector3d pos = *atom->pos();
      
      switch (index.column()) {
      case 0: {// atomic number
        // Try first as a number
        bool ok;
        int atomicNumber = value.toInt(&ok);
        if (ok)
          atom->setAtomicNumber(atomicNumber);
        else
          atom->setAtomicNumber(OpenBabel::etab.GetAtomicNum(value.toString().toAscii()));

        m_molecule->update();
        emit dataChanged(index, index);
        return true;
      }
      case 3: {// formal charge
        bool ok;
        int formalCharge = value.toInt(&ok);
        if (ok)
          atom->setFormalCharge(formalCharge);
      }
      case 4: // partial charge
        atom->setPartialCharge(value.toDouble());
        m_molecule->update();
        emit dataChanged(index, index);
        return true;
      case 5:
      case 6:
      case 7:
        pos[index.column()-5] = value.toDouble();
        atom->setPos(pos);
        m_molecule->update();
        emit dataChanged(index, index);
        return true;      
      case 1: // type
      case 2: // valence
      default:
        return false;
      }
    }
    /*else if (m_type == CartesianType) {
      if (index.column() > 2)
        return false;

      Atom *atom = m_molecule->atom(index.row());
      Eigen::Vector3d pos = *atom->pos();
      pos[index.column()] = value.toDouble();
      atom->setPos(pos);

      m_molecule->update();
      emit dataChanged(index, index);
      return true;
    }*/
    else if (m_type == BondType) {
      Bond *bond = m_molecule->bond(index.row());
      Eigen::Vector3d bondDirection = *(bond->beginPos()) - *(bond->endPos());
      double lengthScale;
      SkeletonTree zMatrixTree;

      switch (index.column()) {
      case 4: // length
        lengthScale = (value.toDouble() - bond->length()) / bond->length();
        // scale our bond vector to match the new length
        bondDirection *= lengthScale;

        zMatrixTree.populate(bond->beginAtom(), bond, m_molecule);
        zMatrixTree.skeletonTranslate(bondDirection);
        emit dataChanged(index, index);
        return true;

        break;
      default:
        return false;
      }
    }
    else if (m_type == AngleType) {
      OBAngleData *ad = static_cast<OBAngleData *>(m_cachedOBMol->GetData(AngleData));
      vector<vector<unsigned int> > angles;
      ad->FillAngleArray(angles);

      Atom *startAtom = m_molecule->atom((angles[index.row()][1]));
      Atom *vertex = m_molecule->atom((angles[index.row()][0]));
      Atom *endAtom = m_molecule->atom((angles[index.row()][2]));
      Bond *bond = startAtom->bond(vertex);
      SkeletonTree zMatrixTree;
      Eigen::Vector3d abVector, bcVector, crossProductVector;
      double rotationAdjustment;
      double initialAngle = m_cachedOBMol->GetAngle(m_cachedOBMol->GetAtom(angles[index.row()][1] + 1),
                                      m_cachedOBMol->GetAtom(angles[index.row()][0] + 1),
                                      m_cachedOBMol->GetAtom(angles[index.row()][2] + 1));
      if (numeric_limits<double>::has_infinity &&
          initialAngle == numeric_limits<double>::infinity()) {
        initialAngle = 0.0;
      }

      switch (index.column()) {
      case 3: // angle
        abVector = *(startAtom->pos()) - *(vertex->pos());
        bcVector = *(endAtom->pos()) - *(vertex->pos());
        crossProductVector = abVector.cross(bcVector).normalized();

        rotationAdjustment = (value.toDouble() - initialAngle) * cDegToRad;

        zMatrixTree.populate(vertex, bond, m_molecule);
        zMatrixTree.skeletonRotate(rotationAdjustment, crossProductVector, *(vertex->pos()));

        emit dataChanged(index, index);
        return true;
        break;
      default:
        return false;
      }
    }
    else if (m_type == TorsionType) {
      OBTorsionData *td = static_cast<OBTorsionData *>(m_cachedOBMol->GetData(TorsionData));
      vector<vector<unsigned int> > torsions;
      td->FillTorsionArray(torsions);

      // Dihedral angles (torsions) are defined like so:
      // a
      //  \b-c
      //      \d

      Atom *b = m_molecule->atom((torsions[index.row()][1]));
      Atom *c = m_molecule->atom((torsions[index.row()][2]));
      Bond *bond = b->bond(c);
      SkeletonTree zMatrixTree;
      Eigen::Vector3d bcVector;
      double rotationAdjustment;
      double initialAngle = m_cachedOBMol->GetTorsion(m_cachedOBMol->GetAtom(torsions[index.row()][0] + 1),
                                      m_cachedOBMol->GetAtom(torsions[index.row()][1] + 1),
                                      m_cachedOBMol->GetAtom(torsions[index.row()][2] + 1),
                                      m_cachedOBMol->GetAtom(torsions[index.row()][3] + 1));
      if (numeric_limits<double>::has_infinity &&
          initialAngle == numeric_limits<double>::infinity()) {
        initialAngle = 0.0;
      }

      switch (index.column()) {
      case 4: // dihedral angle
        bcVector = (*(b->pos()) - *(c->pos())).normalized();
        rotationAdjustment = (value.toDouble() - initialAngle) * cDegToRad;

        zMatrixTree.populate(b, bond, m_molecule);
        zMatrixTree.skeletonRotate(rotationAdjustment, bcVector, *(b->pos()));

        emit dataChanged(index, index);
        return true;
        break;
      default:
        return false;
      }
    }
    return false;
  }

  void PropertiesModel::setMolecule(Molecule *molecule)
  {
    m_molecule = molecule;
    m_validCache = false;
  }

  void PropertiesModel::cacheOBMol() const
  {
    if (m_cachedOBMol)
      delete m_cachedOBMol;

    m_cachedOBMol = new OBMol(m_molecule->OBMol());

    m_validCache = true;
  }

  void PropertiesModel::atomAdded(Atom *atom)
  {
    if ( (m_type == AtomType) /*|| (m_type == CartesianType)*/ ) {
      // insert a new row at the end
      beginInsertRows(QModelIndex(), atom->index(), atom->index());
      endInsertRows();
    }
    m_validCache = false;
  }

  void PropertiesModel::atomRemoved(Atom *atom)
  {
    if ( (m_type == AtomType) /*|| (m_type == CartesianType)*/ )  {
      // delete the row for this atom
      beginRemoveRows(QModelIndex(), atom->index(), atom->index());
      endRemoveRows();
    }
    m_validCache = false;
  }

  void PropertiesModel::bondAdded(Bond *bond)
  {
    if ( (m_type == BondType) ) {
      // insert a new row at the end
      beginInsertRows(QModelIndex(), bond->index(), bond->index());
      endInsertRows();
    }
    m_validCache = false;
  }

  void PropertiesModel::bondRemoved(Bond *bond)
  {
    if ( (m_type == BondType) )  {
      // delete the row for this atom
      beginRemoveRows(QModelIndex(), bond->index(), bond->index());
      endRemoveRows();
    }
    m_validCache = false;
  }

  void PropertiesModel::moleculeChanged()
  {
    // Tear down the model and build it back up again
    // FIXME I think this is pretty hackish - is there a better way to handle it?
    //  We cannot know how many rows have been added or removed, just that it
    // was a big number and the molecule changed significantly.
    int rows = rowCount();
    for (int i = 0; i < rows; ++i) {
      beginRemoveRows(QModelIndex(), 0, 0);
      endRemoveRows();
    }
    beginInsertRows(QModelIndex(), 0, rowCount()-1);
    endInsertRows();

    m_validCache = false;
  }

  void PropertiesModel::updateTable()
  {
    emit dataChanged(QAbstractItemModel::createIndex(0, 0),
                     QAbstractItemModel::createIndex(rowCount(), columnCount()));

    m_validCache = false;
  }

} // end namespace Avogadro

