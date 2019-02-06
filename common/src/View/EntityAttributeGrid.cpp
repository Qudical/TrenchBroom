/*
 Copyright (C) 2010-2017 Kristian Duske
 
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

#include "EntityAttributeGrid.h"

#include "Model/EntityAttributes.h"
#include "Model/Object.h"
#include "View/BorderLine.h"
#include "View/EntityAttributeGridTable.h"
#include "View/ViewConstants.h"
#include "View/MapDocument.h"
#include "View/wxUtils.h"

#include <QHeaderView>
#include <QTableView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QAbstractButton>
#include <QShortcut>
#include <QKeySequence>
#include <QDebug>
#include <QTextEdit>
#include <QKeyEvent>

namespace TrenchBroom {
    namespace View {
        EntityAttributeGrid::EntityAttributeGrid(QWidget* parent, MapDocumentWPtr document) :
        QWidget(parent),
        m_document(document),
        m_ignoreSelection(false) {
            createGui(document);
            createShortcuts();
            updateShortcuts();
            bindObservers();
        }
        
        EntityAttributeGrid::~EntityAttributeGrid() {
            unbindObservers();
        }
        
        void EntityAttributeGrid::addAttribute() {
            qDebug("FIXME: addAttribute");
//            m_grid->InsertRows(m_table->GetNumberAttributeRows());
//            m_grid->SetFocus();
//            const int row = m_table->GetNumberAttributeRows() - 1;
//            m_grid->SelectRow(row);
//            m_grid->GoToCell(row, 0);
//            m_grid->ShowCellEditControl();



            m_grid->setFocus();
            MapDocumentSPtr document = lock(m_document);

            document->setAttribute("new attribute", "");
        }
        
        void EntityAttributeGrid::removeSelectedAttributes() {
            qDebug("FIXME: removeSelectedAttributes");

            QItemSelectionModel *s = m_grid->selectionModel();
            if (!s->hasSelection()) {
                return;
            }
            // FIXME: support more than 1 row
            // FIXME: current vs selected
            QModelIndex current = s->currentIndex();
            if (!current.isValid()) {
                return;
            }

            const AttributeRow* temp = m_table->dataForModelIndex(current);
            String name = temp->name();

            MapDocumentSPtr document = lock(m_document);

            // FIXME: transaction
            document->removeAttribute(name);


//            assert(canRemoveSelectedAttributes());
//
//            const auto selectedRows = selectedRowsAndCursorRow();
//
//            StringList attributes;
//            for (const int row : selectedRows) {
//                attributes.push_back(m_table->attributeName(row));
//            }
//
//            for (const String& key : attributes) {
//                removeAttribute(key);
//            }
        }
        
        /**
         * Removes an attribute, and clear the current selection.
         *
         * If this attribute is still in the table after removing, sets the grid cursor on the new row
         */
        void EntityAttributeGrid::removeAttribute(const String& key) {
            qDebug() << "removeAttribute " << QString::fromStdString(key);




//            const int row = m_table->rowForName(key);
//            if (row == -1)
//                return;
//
//            m_grid->DeleteRows(row, 1);
//            m_grid->ClearSelection();
//
//            const int newRow = m_table->rowForName(key);
//            if (newRow != -1) {
//                m_grid->SetGridCursor(newRow, m_grid->GetGridCursorCol());
//            }
        }

        bool EntityAttributeGrid::canRemoveSelectedAttributes() const {
            return true;

            const auto rows = selectedRowsAndCursorRow();
            if (rows.empty())
                return false;
            
            for (const int row : rows) {
//                if (!m_table->canRemove(row))
//                    return false;
            }
            return true;
        }

        std::set<int> EntityAttributeGrid::selectedRowsAndCursorRow() const {
            std::set<int> result;

            // FIXME:
//            if (m_grid->GetGridCursorCol() != -1
//                && m_grid->GetGridCursorRow() != -1) {
//                result.insert(m_grid->GetGridCursorRow());
//            }
//
//            for (const int row : m_grid->GetSelectedRows()) {
//                result.insert(row);
//            }
            return result;
        }

#if 0
        /**
         * Subclass of wxGridCellTextEditor for setting up autocompletion
         */
        class EntityAttributeCellEditor : public wxGridCellTextEditor
        {
        private:
            EntityAttributeGrid* m_grid;
            EntityAttributeGridTable* m_table;
            int m_row, m_col;
            bool m_forceChange;
            String m_forceChangeAttribute;
            
        public:
            EntityAttributeCellEditor(EntityAttributeGrid* grid, EntityAttributeGridTable* table)
            : m_grid(grid),
            m_table(table),
            m_row(-1),
            m_col(-1),
            m_forceChange(false),
            m_forceChangeAttribute("") {}

        private:
            void OnCharHook(wxKeyEvent& event) {
                if (event.GetKeyCode() == WXK_TAB) {
                    // HACK: Consume tab key and use it for cell navigation.
                    // Otherwise, wxTextCtrl::AutoComplete uses it for cycling between completions (on Windows)
                    
                    // First, close the cell editor
                    m_grid->gridWindow()->DisableCellEditControl();
                    
                    // Closing the editor might reorder the cells (#2094), so m_row/m_col are no longer valid.
                    // Ask the wxGrid for the cursor row/column.
                    m_grid->tabNavigate(m_grid->gridWindow()->GetGridCursorRow(), m_grid->gridWindow()->GetGridCursorCol(), !event.ShiftDown());
                } else if (event.GetKeyCode() == WXK_RETURN && m_col == 1) {
                    // HACK: (#1976) Make the next call to EndEdit return true unconditionally
                    // so it's possible to press enter to apply a value to all entites in a selection
                    // even though the grid editor hasn't changed.

                    const TemporarilySetBool forceChange{m_forceChange};
                    const TemporarilySetAny<String> forceChangeAttribute{m_forceChangeAttribute, m_table->attributeName(m_row)};
                        
                    m_grid->gridWindow()->SaveEditControlValue();
                    m_grid->gridWindow()->HideCellEditControl();
                } else {
                    event.Skip();
                }
            }

        public:
            void BeginEdit(int row, int col, wxGrid* grid) override {
                wxGridCellTextEditor::BeginEdit(row, col, grid);
                assert(grid == m_grid->gridWindow());

                m_row = row;
                m_col = col;
                
                wxTextCtrl *textCtrl = Text();
                ensure(textCtrl != nullptr, "wxGridCellTextEditor::Create should have created control");

                const wxArrayString completions = m_table->getCompletions(row, col);
                textCtrl->AutoComplete(completions);

                textCtrl->Bind(wxEVT_CHAR_HOOK, &EntityAttributeCellEditor::OnCharHook, this);
            }
            
            bool EndEdit(int row, int col, const wxGrid* grid, const QString& oldval, QString *newval) override {
                assert(grid == m_grid->gridWindow());
                
                wxTextCtrl *textCtrl = Text();
                ensure(textCtrl != nullptr, "wxGridCellTextEditor::Create should have created control");
                
                textCtrl->Unbind(wxEVT_CHAR_HOOK, &EntityAttributeCellEditor::OnCharHook, this);
                
                const bool superclassDidChange = wxGridCellTextEditor::EndEdit(row, col, grid, oldval, newval);

                const String changedAttribute = m_table->attributeName(row);
                
                if (m_forceChange
                    && col == 1
                    && m_forceChangeAttribute == changedAttribute) {
                    return true;
                } else {
                    return superclassDidChange;
                }
            }
            
            void ApplyEdit(int row, int col, wxGrid* grid) override {
                if (col == 0) {
                    // Hack to preserve selection when renaming a key (#2094)
                    const auto newName = GetValue().ToStdString();
                    m_grid->setLastSelectedNameAndColumn(newName, col);
                }
                wxGridCellTextEditor::ApplyEdit(row, col, grid);
            }
        };
#endif

        class MyTable : public QTableView {
        protected:
            bool event(QEvent *event) override {
                if (event->type() == QEvent::ShortcutOverride) {
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

                    if (keyEvent->key() < Qt::Key_Escape &&
                        (keyEvent->modifiers() == Qt::NoModifier || keyEvent->modifiers() == Qt::KeypadModifier)) {
                        qDebug("overriding shortcut key %d\n", keyEvent->key());
                        event->setAccepted(true);
                        return true;
                    } else {
                        qDebug("not overriding shortcut key %d\n", keyEvent->key());
                    }

                }
                return QTableView::event(event);
            }
        };

        void EntityAttributeGrid::createGui(MapDocumentWPtr document) {
            m_table = new EntityAttributeGridTable(document, this);
            
            m_grid = new MyTable();
            m_grid->setModel(m_table);
            m_grid->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            m_grid->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            m_grid->setSelectionBehavior(QAbstractItemView::SelectItems);

//            m_grid->Bind(wxEVT_GRID_SELECT_CELL, &EntityAttributeGrid::OnAttributeGridSelectCell, this);

            m_addAttributeButton = createBitmapButton(this, "Add.png", tr("Add a new property"));
            connect(m_addAttributeButton, &QAbstractButton::clicked, this, [=](bool checked){
                addAttribute();
            });

            m_removePropertiesButton = createBitmapButton(this, "Remove.png", tr("Remove the selected properties"));
            connect(m_removePropertiesButton, &QAbstractButton::clicked, this, [=](bool checked){
                removeSelectedAttributes();
            });

            m_showDefaultPropertiesCheckBox = new QCheckBox(tr("Show default properties"));
            connect(m_showDefaultPropertiesCheckBox, &QCheckBox::stateChanged, this, [=](int state){
                //m_table->setShowDefaultRows(state == Qt::Checked);
            });

            // Shortcuts

            auto* buttonSizer = new QHBoxLayout();
            buttonSizer->addWidget(m_addAttributeButton, 0, Qt::AlignVCenter);
            buttonSizer->addWidget(m_removePropertiesButton, 0, Qt::AlignVCenter);
            buttonSizer->addSpacing(LayoutConstants::WideHMargin);
            buttonSizer->addWidget(m_showDefaultPropertiesCheckBox, 0, Qt::AlignVCenter);
            buttonSizer->addStretch(1);

            auto* te = new QTextEdit();
            
            auto* sizer = new QVBoxLayout();
            sizer->setContentsMargins(0, 0, 0, 0);
            sizer->addWidget(m_grid, 1);
            sizer->addWidget(new BorderLine(nullptr, BorderLine::Direction_Horizontal), 0);
            sizer->addLayout(buttonSizer, 0);
            sizer->addWidget(te);
            setLayout(sizer);

            printf("et: %d\n", m_grid->editTriggers());

            m_grid->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);
        }

        void EntityAttributeGrid::createShortcuts() {
            m_insertRowShortcut = new QShortcut(QKeySequence("Ctrl-Return"), this);
            m_insertRowShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_insertRowShortcut, &QShortcut::activated, this, [=](){
                addAttribute();
            });

            m_removeRowShortcut = new QShortcut(QKeySequence("Delete"), this);
            m_removeRowShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_removeRowShortcut, &QShortcut::activated, this, [=](){
                removeSelectedAttributes();
            });

            m_removeRowAlternateShortcut = new QShortcut(QKeySequence("Backspace"), this);
            m_removeRowAlternateShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_removeRowAlternateShortcut, &QShortcut::activated, this, [=](){
                removeSelectedAttributes();
            });

            m_openCellEditorShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);// "Enter"), this);
            m_openCellEditorShortcut->setContext(Qt::WidgetWithChildrenShortcut);
            connect(m_openCellEditorShortcut, &QShortcut::activated, this, [=](){
                qDebug("enter activated unambiguously");
                m_grid->edit(m_grid->currentIndex());
            });
            connect(m_openCellEditorShortcut, &QShortcut::activatedAmbiguously, this, [=](){
                qDebug("enter activated ambiguously");
                m_grid->edit(m_grid->currentIndex());
            });
       }

       void EntityAttributeGrid::updateShortcuts() {
           m_insertRowShortcut->setEnabled(true);
           m_removeRowShortcut->setEnabled(canRemoveSelectedAttributes());
           m_removeRowAlternateShortcut->setEnabled(canRemoveSelectedAttributes());
           // FIXME:
           //m_openCellEditorShortcut->setEnabled(m_grid->CanEnableCellControl() && !m_grid->IsCellEditControlShown());
        }

        void EntityAttributeGrid::bindObservers() {
            MapDocumentSPtr document = lock(m_document);
            document->documentWasNewedNotifier.addObserver(this, &EntityAttributeGrid::documentWasNewed);
            document->documentWasLoadedNotifier.addObserver(this, &EntityAttributeGrid::documentWasLoaded);
            document->nodesDidChangeNotifier.addObserver(this, &EntityAttributeGrid::nodesDidChange);
            document->selectionWillChangeNotifier.addObserver(this, &EntityAttributeGrid::selectionWillChange);
            document->selectionDidChangeNotifier.addObserver(this, &EntityAttributeGrid::selectionDidChange);
        }
        
        void EntityAttributeGrid::unbindObservers() {
            if (!expired(m_document)) {
                MapDocumentSPtr document = lock(m_document);
                document->documentWasNewedNotifier.removeObserver(this, &EntityAttributeGrid::documentWasNewed);
                document->documentWasLoadedNotifier.removeObserver(this, &EntityAttributeGrid::documentWasLoaded);
                document->nodesDidChangeNotifier.removeObserver(this, &EntityAttributeGrid::nodesDidChange);
                document->selectionWillChangeNotifier.removeObserver(this, &EntityAttributeGrid::selectionWillChange);
                document->selectionDidChangeNotifier.removeObserver(this, &EntityAttributeGrid::selectionDidChange);
            }
        }
        
        void EntityAttributeGrid::documentWasNewed(MapDocument* document) {
            updateControls();
        }
        
        void EntityAttributeGrid::documentWasLoaded(MapDocument* document) {
            updateControls();
        }
        
        void EntityAttributeGrid::nodesDidChange(const Model::NodeList& nodes) {
            updateControls();
        }
        
        void EntityAttributeGrid::selectionWillChange() {
            // FIXME: Needed?
//            m_grid->SaveEditControlValue();
//            m_grid->HideCellEditControl();
        }
        
        void EntityAttributeGrid::selectionDidChange(const Selection& selection) {
            const TemporarilySetBool ignoreSelection(m_ignoreSelection);
            updateControls();
        }

        void EntityAttributeGrid::updateControls() {
            // FIXME:
            m_table->updateFromMapDocument();

            // FIXME: not sure about this stuff?
#if 0
            int row = m_table->rowForName(m_lastSelectedName);
            if (row >= m_table->GetNumberRows())
                row = m_table->GetNumberRows() - 1;
            if (row == -1 && m_table->GetNumberRows() > 0)
                row = 0;

            if (row != -1) {
                // 1981: Ensure that we make a cell visible only if it is completely invisible.
                // The goal is to block the grid from redrawing itself every time this function
                // is called.
                if (!m_grid->IsVisible(row, m_lastSelectedCol, false)) {
                    m_grid->MakeCellVisible(row, m_lastSelectedCol);
                }
                if (m_grid->GetGridCursorRow() != row || m_grid->GetGridCursorCol() != m_lastSelectedCol) {
                    m_grid->SetGridCursor(row, m_lastSelectedCol);
                }
                if (!m_grid->IsInSelection(row, m_lastSelectedCol)) {
                    m_grid->SelectRow(row);
                }
            } else {
                fireSelectionEvent(row, m_lastSelectedCol);
            }
#endif

            // Update buttons/checkboxes
            MapDocumentSPtr document = lock(m_document);
            m_grid->setEnabled(!document->allSelectedAttributableNodes().empty());
            m_addAttributeButton->setEnabled(!document->allSelectedAttributableNodes().empty());
            m_removePropertiesButton->setEnabled(canRemoveSelectedAttributes());
            //m_showDefaultPropertiesCheckBox->setChecked(m_table->showDefaultRows());

            // Update shortcuts
            updateShortcuts();
        }

        Model::AttributeName EntityAttributeGrid::selectedRowName() const {
            return "";
            // FIXME:
#if 0
            wxArrayInt selectedRows = m_grid->GetSelectedRows();
            if (selectedRows.empty())
                return "";
            const int row = selectedRows.front();
            return m_table->attributeName(row);
#endif
        }
    }
}
