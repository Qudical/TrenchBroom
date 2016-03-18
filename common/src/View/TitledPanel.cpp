/*
 Copyright (C) 2010-2016 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TitledPanel.h"

#include "View/BorderLine.h"
#include "View/TitleBar.h"
#include "View/ViewConstants.h"

#include <wx/sizer.h>

namespace TrenchBroom {
    namespace View {
        TitledPanel::TitledPanel(wxWindow* parent, const wxString& title, const bool showDivider) :
        wxPanel(parent),
        m_panel(new wxPanel(this)) {
            const int hMargin = showDivider ? LayoutConstants::NarrowHMargin : 0;
            const int vMargin = showDivider ? LayoutConstants::NarrowVMargin : 0;
            
            wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(new TitleBar(this, title, hMargin, vMargin), 0, wxEXPAND);
            if (showDivider)
                sizer->Add(new BorderLine(this, BorderLine::Direction_Horizontal), 0, wxEXPAND);
            sizer->Add(m_panel, 1, wxEXPAND);
            SetSizer(sizer);
        }
        
        wxWindow* TitledPanel::getPanel() const {
            return m_panel;
        }
    }
}
