/**********************************************************************
  OverlayEngine - Engine for displaying overlays.

  Copyright (C) 2008 Tim Vandermeersch

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  Avogadro is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Avogadro is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#ifndef OVERLAYENGINE_H
#define OVERLAYENGINE_H

#include <avogadro/global.h>
#include <avogadro/engine.h>

namespace Avogadro {

  //! Overlay Engine class.
  class OverlayEngine : public Engine
  {
    Q_OBJECT
    AVOGADRO_ENGINE("Overlay", tr("Overlay"),
                    tr("Renders color scale for gradients"))

    public:
      //! Constructor
      OverlayEngine(QObject *parent=0);

      Engine *clone() const;

      //! Deconstructor
      ~OverlayEngine() {}

      //! \name Render Methods
      //@{
      bool renderOpaque(PainterDevice *pd);
      bool renderQuick(PainterDevice *pd);
      //@}

      Layers layers() const;
      PrimitiveTypes primitiveTypes() const;
      ColorTypes colorTypes() const;

      /**
       * Write the engine settings so that they can be saved between sessions.
       */
      void writeSettings(QSettings &settings) const;

      /**
       * Read in the settings that have been saved for the engine instance.
       */
      void readSettings(QSettings &settings);

  };

  //! Generates instances of our OverlayEngine class
  class OverlayEngineFactory : public QObject, public PluginFactory
  {
    Q_OBJECT
    Q_INTERFACES(Avogadro::PluginFactory)
    AVOGADRO_ENGINE_FACTORY(OverlayEngine)
  };

} // end namespace Avogadro

#endif
