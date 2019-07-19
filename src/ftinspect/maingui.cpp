// maingui.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "maingui.hpp"
#include "rendering/grid.hpp"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QtDebug>

#include FT_DRIVER_H
#include FT_TRUETYPE_TABLES_H


MainGUI::MainGUI()
{
  engine = NULL;

  fontWatcher = new QFileSystemWatcher;
  // if the current input file is invalid we retry once a second to load it
  timer = new QTimer;
  timer->setInterval(1000);

  setGraphicsDefaults();
  createLayout();
  createConnections();
  createActions();
  createMenus();
  createStatusBar();

  readSettings();

  setUnifiedTitleAndToolBarOnMac(true);
}


MainGUI::~MainGUI()
{
  // empty
}


void
MainGUI::update(Engine* e)
{
  engine = e;
}


// overloading

void
MainGUI::closeEvent(QCloseEvent* event)
{
  writeSettings();
  event->accept();
}


void
MainGUI::showFontName()
{
    QMessageBox::about(
      this,
      tr("Font Name Entries"),
      tr("<b> Face number : %1 </b><br><br>"
         "Family : %2 <br>"
         "Style : %3 <br>"
         "Postscript : %4")
         .arg(fontList.size())
         .arg(engine->currentFamilyName())
         .arg(engine->currentStyleName())
         .arg(engine->currentPostscriptName()));
}


void
MainGUI::showFontType()
{
  FT_Face face = engine->getFtSize()->face;
  const char* fontType;
  if (FT_IS_SCALABLE( face ) != 0 )
  {
    if (FT_HAS_MULTIPLE_MASTERS( face ) != 0 )
    {
      fontType = "scalable, multiple masters";
    }
    else
    {
      fontType =  "scalable";
    }
  }
  else if (FT_HAS_FIXED_SIZES( face ) != 0)
  {
    fontType = "fixed size";
  }

  const char* direction = "" ;
  if (FT_HAS_HORIZONTAL( face))
  {
    if (FT_HAS_VERTICAL( face ))
    {
      direction =  " horizontal vertical";
    }
    else
    {
      direction = " horizontal";
    }
  }
  else
  {
    if (FT_HAS_VERTICAL( face ))
    {
      direction =  " vertical";
    }
  }

  if(FT_IS_SCALABLE( face ))
  {
    QMessageBox::about(this,
      tr("Font Type Entries"),
      tr("<p><b>Face number : %1</b><br><br>"
          "FreeType driver: %2<br>"
          "sfnt wrapped : %3<br>"
          "type : %4<br>"
          "direction : %5 <br>"
          "fixed width : %6 <br>"
          "glyph names : %7 <br></p>"
          "<b>Scalability Properties</b><br>"
          "EM size : %9 <br>"
          "global BBox : (%10, %11):(%12, %13) <br>"
          "ascent : %14 <br>"
          "descent : %15<br>"
          "text height : %16 <br>")
        .arg(fontList.size())
        .arg(engine->DriverName())
        .arg(FT_IS_SFNT( face ) ? QString("yes") : QString("no"))
        .arg(fontType)
        .arg(direction)
        .arg(FT_IS_FIXED_WIDTH(face) ? QString("yes") : QString("no"))
        .arg(FT_HAS_GLYPH_NAMES( face ) ? QString("yes") : QString("no"))
        .arg(face->units_per_EM)
        .arg(face->bbox.xMin)
        .arg(face->bbox.yMin)
        .arg(face->bbox.xMax)
        .arg(face->bbox.yMax)
        .arg(face->ascender)
        .arg(face->descender)
        .arg(face->height));
  }
  else
  {
    QMessageBox::about(
    this,
    tr("Font Type Entries"),
    tr("<b>Face number : %1</b><br><br>"
        "FreeType driver: %2<br>"
        "sfnt wrapped : %3<br>"
        "type : %4<br>"
        "direction : %5 <br>"
        "fixed width : %6 <br>"
        "glyph names : %7 <br>")
    .arg(fontList.size())
    .arg(engine->DriverName())
    .arg(FT_IS_SFNT( face ) ? QString("yes") : QString("no"))
    .arg(fontType)
    .arg(direction)
    .arg(FT_IS_FIXED_WIDTH(face) ? QString("yes") : QString("no"))
    .arg(FT_HAS_GLYPH_NAMES( face ) ? QString("yes") : QString("no")));
  }
}


void
MainGUI::showCharmapsInfo()
{
  FT_Face face = engine->getFtSize()->face;
  QMessageBox msgBox;
  for(int i = 0 ; i < face->num_charmaps ; i++)
  {
    QMessageBox::about(
      this,
      tr("Charmaps Info"),
      tr("Format : %1<br>"
        "Platform : %2<br>"
        "Encoding : %3<br>"
        "Language : %4<br>")
        .arg(FT_Get_CMap_Format( face->charmaps[i] ))
        .arg(face->charmaps[i]->platform_id)
        .arg(face->charmaps[i]->encoding_id)
        .arg(FT_Get_CMap_Language_ID(face->charmaps[i])));
  }
}


void
MainGUI::about()
{
  QMessageBox::about(
    this,
    tr("About ftinspect"),
    tr("<p>This is <b>ftinspect</b> version %1<br>"
       " Copyright %2 2016-2019<br>"
       " by Werner Lemberg <tt>&lt;wl@gnu.org&gt;</tt></p>"
       ""
       "<p><b>ftinspect</b> shows how a font gets rendered"
       " by FreeType, allowing control over virtually"
       " all rendering parameters.</p>"
       ""
       "<p>License:"
       " <a href='http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/FTL.TXT'>FreeType"
       " License (FTL)</a> or"
       " <a href='http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/GPLv2.TXT'>GNU"
       " GPLv2</a></p>")
       .arg(QApplication::applicationVersion())
       .arg(QChar(0xA9)));
}


void
MainGUI::aboutQt()
{
  QApplication::aboutQt();
}


void
MainGUI::loadFonts()
{
  int oldSize = fontList.size();

  QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        tr("Load one or more fonts"),
                        QDir::homePath(),
                        "",
                        NULL,
                        QFileDialog::ReadOnly);

  // XXX sort data, uniquify elements
  fontList.append(files);

  // Enable font info menu
  menuInfo->setEnabled(true);

  // if we have new fonts, set the current index to the first new one
  if (oldSize < fontList.size())
    currentFontIndex = oldSize;

  showFont();
}


void
MainGUI::closeFont()
{
  if (currentFontIndex < fontList.size())
  {
    engine->removeFont(currentFontIndex);
    fontWatcher->removePath(fontList[currentFontIndex]);
    fontList.removeAt(currentFontIndex);
  }

  // show next font after deletion, i.e., retain index if possible
  if (fontList.size())
  {
    if (currentFontIndex >= fontList.size())
      currentFontIndex = fontList.size() - 1;
  }
  else
    currentFontIndex = 0;

  // disable font info menu
  menuInfo->setEnabled(false);

  showFont();
}


void
MainGUI::watchCurrentFont()
{
  timer->stop();
  showFont();
}


void
MainGUI::showFont()
{
  // we do lazy computation of FT_Face objects

  if (currentFontIndex < fontList.size())
  {
    QString& font = fontList[currentFontIndex];
    QFileInfo fileInfo(font);
    QString fontName = fileInfo.fileName();

    if (fileInfo.exists())
    {
      // Qt's file watcher doesn't handle symlinks;
      // we thus fall back to polling
      if (fileInfo.isSymLink())
      {
        fontName.prepend("<i>");
        fontName.append("</i>");
        timer->start();
      }
      else
        fontWatcher->addPath(font);
    }
    else
    {
      // On Unix-like systems, the symlink's target gets opened; this
      // implies that deletion of a symlink doesn't make `engine->loadFont'
      // fail since it operates on a file handle pointing to the target.
      // For this reason, we remove the font to enforce a reload.
      engine->removeFont(currentFontIndex);
    }

    fontFilenameLabel->setText(fontName);
  }
  else
    fontFilenameLabel->clear();

  currentNumberOfFaces
    = engine->numberOfFaces(currentFontIndex);
  currentNumberOfNamedInstances
    = engine->numberOfNamedInstances(currentFontIndex,
                                     currentFaceIndex);
  currentNumberOfGlyphs
    = engine->loadFont(currentFontIndex,
                       currentFaceIndex,
                       currentNamedInstanceIndex);

  if (currentNumberOfGlyphs < 0)
  {
    // there might be various reasons why the current
    // (file, face, instance) triplet is invalid or missing;
    // we thus start our timer to periodically test
    // whether the font starts working
    if (currentFontIndex < fontList.size())
      timer->start();
  }

  fontNameLabel->setText(QString("%1 %2")
                         .arg(engine->currentFamilyName())
                         .arg(engine->currentStyleName()));

  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
  checkHinting();
  adjustGlyphIndex(0);

  drawGlyph();
}


void
MainGUI::checkHinting()
{
  if (hintingCheckBox->isChecked())
  {
    if (engine->fontType == Engine::FontType_CFF)
    {
      for (int i = 0; i < hintingModeComboBoxx->count(); i++)
      {
        if (hintingModesCFFHash.key(i, -1) != -1)
          hintingModeComboBoxx->setItemEnabled(i, true);
        else
          hintingModeComboBoxx->setItemEnabled(i, false);
      }

      hintingModeComboBoxx->setCurrentIndex(currentCFFHintingMode);
    }
    else if (engine->fontType == Engine::FontType_TrueType)
    {
      for (int i = 0; i < hintingModeComboBoxx->count(); i++)
      {
        if (hintingModesTrueTypeHash.key(i, -1) != -1)
          hintingModeComboBoxx->setItemEnabled(i, true);
        else
          hintingModeComboBoxx->setItemEnabled(i, false);
      }

      hintingModeComboBoxx->setCurrentIndex(currentTTInterpreterVersion);
    }
    else
    {
      hintingModeLabel->setEnabled(false);
      hintingModeComboBoxx->setEnabled(false);
    }

    for (int i = 0; i < hintingModesAlwaysDisabled.size(); i++)
      hintingModeComboBoxx->setItemEnabled(hintingModesAlwaysDisabled[i],
                                           false);

    autoHintingCheckBox->setEnabled(true);
    checkAutoHinting();
  }
  else
  {
    hintingModeLabel->setEnabled(false);
    hintingModeComboBoxx->setEnabled(false);

    autoHintingCheckBox->setEnabled(false);
    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, false);
  }

  drawGlyph();
}


void
MainGUI::checkRenderingMode()
{
  int index = renderingModeComboBoxx->currentIndex();

  renderingModeComboBoxx->setItemEnabled(index, true);

  if (!(renderingModeComboBoxx->itemText(index).compare("Normal")))
  {
    render_mode = 1;
  } else if (!(renderingModeComboBoxx->itemText(index).compare("Fancy")))
  {
    render_mode = 2;
  } else if (!(renderingModeComboBoxx->itemText(index).compare("Stroked")))
  {
    render_mode = 3;
  } else if (!(renderingModeComboBoxx->itemText(index).compare("Text String")))
  {
    render_mode = 4;
  } else if (!(renderingModeComboBoxx->itemText(index).compare("Waterfall")))
  {
    render_mode = 5;
    glyphView->setDragMode(QGraphicsView::NoDrag);
  } else
  {
    render_mode = 6;
  }
  renderAll();
}

void
MainGUI::checkKerningMode()
{
  int index = kerningModeComboBoxx->currentIndex();
  kerningModeComboBoxx->setItemEnabled(index, true);

  if (!(kerningModeComboBoxx->itemText(index).compare("No Kerning")))
  {
    kerning_mode = 0;
  } else if (!(kerningModeComboBoxx->itemText(index).compare("Normal")))
  {
    kerning_mode = 1;
  } else if (!(kerningModeComboBoxx->itemText(index).compare("Smart")))
  {
    kerning_mode = 2;
  }
  
  render_mode = 6;
  renderAll();
  renderingModeComboBoxx->setItemEnabled(5, true);
}


void
MainGUI::checkKerningDegree()
{
  int index = kerningDegreeComboBoxx->currentIndex();
  kerningDegreeComboBoxx->setItemEnabled(index, true);

  if (!(kerningDegreeComboBoxx->itemText(index).compare("None")))
  {
    kerning_degree = 0;
  } else if (!(kerningDegreeComboBoxx->itemText(index).compare("Light")))
  {
    kerning_degree = 1;
  } else if (!(kerningDegreeComboBoxx->itemText(index).compare("Medium")))
  {
    kerning_degree = 2;
  } else if (!(kerningDegreeComboBoxx->itemText(index).compare("Tight")))
  {
    kerning_degree = 3;
  }

  render_mode = 6;
  renderAll();
  renderingModeComboBoxx->setItemEnabled(5, true);
}


void
MainGUI::checkHintingMode()
{
  int index = hintingModeComboBoxx->currentIndex();

  if (engine->fontType == Engine::FontType_CFF)
  {
    engine->setCFFHintingMode(index);
    currentCFFHintingMode = index;
  }
  else if (engine->fontType == Engine::FontType_TrueType)
  {
    engine->setTTInterpreterVersion(index);
    currentTTInterpreterVersion = index;
  }

  // this enforces reloading of the font
  showFont();
}


void
MainGUI::checkAutoHinting()
{
  if (autoHintingCheckBox->isChecked())
  {
    hintingModeLabel->setEnabled(false);
    hintingModeComboBoxx->setEnabled(false);

    horizontalHintingCheckBox->setEnabled(true);
    verticalHintingCheckBox->setEnabled(true);
    blueZoneHintingCheckBox->setEnabled(true);
    segmentDrawingCheckBox->setEnabled(true);
    if (engine->haveWarping)
      warpingCheckBox->setEnabled(true);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, true);
  }
  else
  {
    if (engine->fontType == Engine::FontType_CFF
        || engine->fontType == Engine::FontType_TrueType)
    {
      hintingModeLabel->setEnabled(true);
      hintingModeComboBoxx->setEnabled(true);
    }

    horizontalHintingCheckBox->setEnabled(false);
    verticalHintingCheckBox->setEnabled(false);
    blueZoneHintingCheckBox->setEnabled(false);
    segmentDrawingCheckBox->setEnabled(false);
    warpingCheckBox->setEnabled(false);

    antiAliasingComboBoxx->setItemEnabled(AntiAliasing_Light, false);

    if (antiAliasingComboBoxx->currentIndex() == AntiAliasing_Light)
      antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_Normal);
  }

  drawGlyph();
}


void
MainGUI::gridViewRender()
{
  if (gridView->isChecked())
  {
    if (currentRenderAllItem)
    {
      glyphScene->removeItem(currentRenderAllItem);
      delete currentRenderAllItem;

      currentRenderAllItem = NULL;
    }

    currentGridItem = new Grid(gridPen, axisPen);
    glyphScene->addItem(currentGridItem);
    zoomSpinBox->setValue(20);
    //drawGlyph();
  }

}


void
MainGUI::renderAll()
{
  // Embolden factors
  double x_factor = embolden_x_Slider->value()/1000.0;
  double y_factor = embolden_y_Slider->value()/1000.0;

  double slant_factor = slant_Slider->value()/100.0;
  double stroke_factor = stroke_Slider->value()/1000.0;

  // Basic definition
  FT_Size size;
  FT_Face face;
  FT_Error error;

  // Basic Initialization
  size = engine->getFtSize();

  // Set the charmap for now
  //error = FT_Set_Charmap( size->face, size->face->charmaps[0]);
  // chache manager
  FTC_Manager cacheManager = engine->cacheManager;
  FTC_CMapCache cmap_cache = engine->cmap_cache;
  FTC_FaceID  face_id = engine->scaler.face_id;
  //face = size->face;

  

  if (currentGridItem)
  {
    glyphScene->removeItem(currentGridItem);
    delete currentGridItem;

    currentGridItem = NULL;
  }

  if (currentRenderAllItem)
  {
    glyphScene->removeItem(currentRenderAllItem);
    delete currentRenderAllItem;

    currentRenderAllItem = NULL;
  }

  /* now, draw to our target surface */
  currentRenderAllItem = new RenderAll(size->face,
                                  size,
                                  cacheManager,
                                  face_id,
                                  cmap_cache,
                                  engine->library,
                                  render_mode,
                                  engine->scaler,
                                  engine->imageCache,
                                  x_factor,
                                  y_factor,
                                  slant_factor,
                                  stroke_factor,
                                  kerning_mode,
                                  kerning_degree);
  glyphScene->addItem(currentRenderAllItem);
  zoomSpinBox->setValue(1);
}


void
MainGUI::checkAntiAliasing()
{
  int index = antiAliasingComboBoxx->currentIndex();

  if (index == AntiAliasing_None
      || index == AntiAliasing_Normal
      || index == AntiAliasing_Light)
  {
    lcdFilterLabel->setEnabled(false);
    lcdFilterComboBox->setEnabled(false);
  }
  else
  {
    lcdFilterLabel->setEnabled(true);
    lcdFilterComboBox->setEnabled(true);
  }

  drawGlyph();
}


void
MainGUI::checkLcdFilter()
{
  int index = lcdFilterComboBox->currentIndex();
  FT_Library_SetLcdFilter(engine->library, lcdFilterHash.key(index));
}


void
MainGUI::checkShowPoints()
{
  if (showPointsCheckBox->isChecked())
    showPointNumbersCheckBox->setEnabled(true);
  else
    showPointNumbersCheckBox->setEnabled(false);

  drawGlyph();
}


void
MainGUI::checkUnits()
{
  int index = unitsComboBox->currentIndex();

  if (index == Units_px)
  {
    dpiLabel->setEnabled(false);
    dpiSpinBox->setEnabled(false);
    sizeDoubleSpinBox->setSingleStep(1);
    sizeDoubleSpinBox->setValue(qRound(sizeDoubleSpinBox->value()));
  }
  else
  {
    dpiLabel->setEnabled(true);
    dpiSpinBox->setEnabled(true);
    sizeDoubleSpinBox->setSingleStep(0.5);
  }

  drawGlyph();
}


void
MainGUI::adjustGlyphIndex(int delta)
{
  // only adjust current glyph index if we have a valid font
  if (currentNumberOfGlyphs > 0)
  {
    currentGlyphIndex += delta;
    currentGlyphIndex = qBound(0,
                               currentGlyphIndex,
                               currentNumberOfGlyphs - 1);
  }

  QString upperHex = QString::number(currentGlyphIndex, 16).toUpper();
  glyphIndexLabel->setText(QString("%1 (0x%2)")
                                   .arg(currentGlyphIndex)
                                   .arg(upperHex));
  glyphNameLabel->setText(engine->glyphName(currentGlyphIndex));

  drawGlyph();
}


void
MainGUI::checkCurrentFontIndex()
{
  if (fontList.size() < 2)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(false);
  }
  else if (currentFontIndex == 0)
  {
    previousFontButton->setEnabled(false);
    nextFontButton->setEnabled(true);
  }
  else if (currentFontIndex >= fontList.size() - 1)
  {
    previousFontButton->setEnabled(true);
    nextFontButton->setEnabled(false);
  }
  else
  {
    previousFontButton->setEnabled(true);
    nextFontButton->setEnabled(true);
  }
}


void
MainGUI::checkCurrentFaceIndex()
{
  if (currentNumberOfFaces < 2)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(false);
  }
  else if (currentFaceIndex == 0)
  {
    previousFaceButton->setEnabled(false);
    nextFaceButton->setEnabled(true);
  }
  else if (currentFaceIndex >= currentNumberOfFaces - 1)
  {
    previousFaceButton->setEnabled(true);
    nextFaceButton->setEnabled(false);
  }
  else
  {
    previousFaceButton->setEnabled(true);
    nextFaceButton->setEnabled(true);
  }
}


void
MainGUI::checkCurrentNamedInstanceIndex()
{
  if (currentNumberOfNamedInstances < 2)
  {
    previousNamedInstanceButton->setEnabled(false);
    nextNamedInstanceButton->setEnabled(false);
  }
  else if (currentNamedInstanceIndex == 0)
  {
    previousNamedInstanceButton->setEnabled(false);
    nextNamedInstanceButton->setEnabled(true);
  }
  else if (currentNamedInstanceIndex >= currentNumberOfNamedInstances - 1)
  {
    previousNamedInstanceButton->setEnabled(true);
    nextNamedInstanceButton->setEnabled(false);
  }
  else
  {
    previousNamedInstanceButton->setEnabled(true);
    nextNamedInstanceButton->setEnabled(true);
  }
}


void
MainGUI::previousFont()
{
  if (currentFontIndex > 0)
  {
    currentFontIndex--;
    currentFaceIndex = 0;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFont()
{
  if (currentFontIndex < fontList.size() - 1)
  {
    currentFontIndex++;
    currentFaceIndex = 0;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousFace()
{
  if (currentFaceIndex > 0)
  {
    currentFaceIndex--;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::nextFace()
{
  if (currentFaceIndex < currentNumberOfFaces - 1)
  {
    currentFaceIndex++;
    currentNamedInstanceIndex = 0;
    showFont();
  }
}


void
MainGUI::previousNamedInstance()
{
  if (currentNamedInstanceIndex > 0)
  {
    currentNamedInstanceIndex--;
    showFont();
  }
}


void
MainGUI::nextNamedInstance()
{
  if (currentNamedInstanceIndex < currentNumberOfNamedInstances - 1)
  {
    currentNamedInstanceIndex++;
    showFont();
  }
}


void
MainGUI::zoom()
{
  int scale = zoomSpinBox->value();

  QTransform transform;
  transform.scale(scale, scale);

  // we want horizontal and vertical 1px lines displayed with full pixels;
  // we thus have to shift the coordinate system accordingly, using a value
  // that represents 0.5px (i.e., half the 1px line width) after the scaling
  qreal shift = 0.5 / scale;
  transform.translate(shift, shift);

  glyphView->setTransform(transform);
  drawGlyph();
}


void
MainGUI::setGraphicsDefaults()
{
  // color tables (with suitable opacity values) for converting
  // FreeType's pixmaps to something Qt understands
  monoColorTable.append(QColor(Qt::transparent).rgba());
  monoColorTable.append(QColor(Qt::black).rgba());

  for (int i = 0xFF; i >= 0; i--)
    grayColorTable.append(qRgba(i, i, i, 0xFF - i));

  // XXX make this user-configurable

  axisPen.setColor(Qt::black);
  axisPen.setWidth(0);
  blueZonePen.setColor(QColor(64, 64, 255, 64)); // light blue
  blueZonePen.setWidthF(0.2);
  gridPen.setColor(Qt::lightGray);
  gridPen.setWidth(0);
  offPen.setColor(Qt::darkGreen);
  offPen.setWidth(3);
  onPen.setColor(Qt::red);
  onPen.setWidth(3);
  outlinePen.setColor(Qt::red);
  outlinePen.setWidth(0);
  segmentPen.setColor(QColor(64, 255, 128, 64)); // light green
  segmentPen.setWidthF(0.2);
}


void
MainGUI::drawGlyph()
{
  // the call to `engine->loadOutline' updates FreeType's load flags
  if (!engine)
    return;

  if (currentGlyphBitmapItem)
  {
    glyphScene->removeItem(currentGlyphBitmapItem);
    delete currentGlyphBitmapItem;

    currentGlyphBitmapItem = NULL;
  }

  if (currentGlyphSegmentItem)
  {
    glyphScene->removeItem(currentGlyphSegmentItem);
    delete currentGlyphSegmentItem;

    currentGlyphSegmentItem = NULL;
  }

  if (currentGlyphOutlineItem)
  {
    glyphScene->removeItem(currentGlyphOutlineItem);
    delete currentGlyphOutlineItem;

    currentGlyphOutlineItem = NULL;
  }

  if (currentGlyphPointsItem)
  {
    glyphScene->removeItem(currentGlyphPointsItem);
    delete currentGlyphPointsItem;

    currentGlyphPointsItem = NULL;
  }

  if (currentGlyphPointNumbersItem)
  {
    glyphScene->removeItem(currentGlyphPointNumbersItem);
    delete currentGlyphPointNumbersItem;

    currentGlyphPointNumbersItem = NULL;
  }

  /* if (currentRenderAllItem)
  {
    glyphScene->removeItem(currentRenderAllItem);
    delete currentRenderAllItem;

    currentRenderAllItem = NULL;

  }*/

  FT_Outline* outline = engine->loadOutline(currentGlyphIndex);
  if (outline)
  {
    if (showBitmapCheckBox->isChecked())
    {
      // XXX support LCD
      FT_Pixel_Mode pixelMode = FT_PIXEL_MODE_GRAY;
      if (antiAliasingComboBoxx->currentIndex() == AntiAliasing_None)
        pixelMode = FT_PIXEL_MODE_MONO;

      currentGlyphBitmapItem = new GlyphBitmap(outline,
                                               engine->library,
                                               pixelMode,
                                               (gammaSlider->value()/10.0),
                                               monoColorTable,
                                               grayColorTable);
      glyphScene->addItem(currentGlyphBitmapItem);
    }

    if (segmentDrawingCheckBox->isChecked())
    {
      currentGlyphSegmentItem = new GlyphSegment(segmentPen,
                                                 blueZonePen,
                                                 engine->getFtSize());
      glyphScene->addItem(currentGlyphSegmentItem);
    }

    if (showOutlinesCheckBox->isChecked())
    {
      currentGlyphOutlineItem = new GlyphOutline(outlinePen, outline);
      glyphScene->addItem(currentGlyphOutlineItem);
    }

    if (showPointsCheckBox->isChecked())
    {
      currentGlyphPointsItem = new GlyphPoints(onPen, offPen, outline);
      glyphScene->addItem(currentGlyphPointsItem);

      if (showPointNumbersCheckBox->isChecked())
      {
        currentGlyphPointNumbersItem = new GlyphPointNumbers(onPen,
                                                             offPen,
                                                             outline);
        glyphScene->addItem(currentGlyphPointNumbersItem);
      }
    }
  }

  glyphScene->update();
}


// XXX distances are specified in pixels,
//     making the layout dependent on the output device resolution
void
MainGUI::createLayout()
{
  // left side
  fontFilenameLabel = new QLabel;

  hintingCheckBox = new QCheckBox(tr("Hinting"));

  hintingModeLabel = new QLabel(tr("Hinting Mode"));
  hintingModeLabel->setAlignment(Qt::AlignRight);
  hintingModeComboBoxx = new QComboBoxx;
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v35,
                                   tr("TrueType v35"));
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v38,
                                   tr("TrueType v38"));
  hintingModeComboBoxx->insertItem(HintingMode_TrueType_v40,
                                   tr("TrueType v40"));
  hintingModeComboBoxx->insertItem(HintingMode_CFF_FreeType,
                                   tr("CFF (FreeType)"));
  hintingModeComboBoxx->insertItem(HintingMode_CFF_Adobe,
                                   tr("CFF (Adobe)"));
  hintingModeLabel->setBuddy(hintingModeComboBoxx);

  autoHintingCheckBox = new QCheckBox(tr("Auto-Hinting"));
  horizontalHintingCheckBox = new QCheckBox(tr("Horizontal Hinting"));
  verticalHintingCheckBox = new QCheckBox(tr("Vertical Hinting"));
  blueZoneHintingCheckBox = new QCheckBox(tr("Blue-Zone Hinting"));
  segmentDrawingCheckBox = new QCheckBox(tr("Segment Drawing"));
  warpingCheckBox = new QCheckBox(tr("Warping"));

  antiAliasingLabel = new QLabel(tr("Anti-Aliasing"));
  antiAliasingLabel->setAlignment(Qt::AlignRight);
  antiAliasingComboBoxx = new QComboBoxx;
  antiAliasingComboBoxx->insertItem(AntiAliasing_None,
                                    tr("None"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_Normal,
                                    tr("Normal"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_Light,
                                    tr("Light"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD,
                                    tr("LCD (RGB)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_BGR,
                                    tr("LCD (BGR)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_Vertical,
                                    tr("LCD (vert. RGB)"));
  antiAliasingComboBoxx->insertItem(AntiAliasing_LCD_Vertical_BGR,
                                    tr("LCD (vert. BGR)"));
  antiAliasingLabel->setBuddy(antiAliasingComboBoxx);

  lcdFilterLabel = new QLabel(tr("LCD Filter"));
  lcdFilterLabel->setAlignment(Qt::AlignRight);
  lcdFilterComboBox = new QComboBox;
  lcdFilterComboBox->insertItem(LCDFilter_Default, tr("Default"));
  lcdFilterComboBox->insertItem(LCDFilter_Light, tr("Light"));
  lcdFilterComboBox->insertItem(LCDFilter_None, tr("None"));
  lcdFilterComboBox->insertItem(LCDFilter_Legacy, tr("Legacy"));
  lcdFilterLabel->setBuddy(lcdFilterComboBox);

  renderingModeLabel = new QLabel(tr("Render Mode"));
  renderingModeLabel->setAlignment(Qt::AlignTop);
  renderingModeComboBoxx = new QComboBoxx;
  renderingModeComboBoxx->insertItem(Normal, tr("Normal"));
  renderingModeComboBoxx->insertItem(Fancy, tr("Fancy"));
  renderingModeComboBoxx->insertItem(Stroked, tr("Stroked"));
  renderingModeComboBoxx->insertItem(Text_String, tr("Text String"));
  renderingModeComboBoxx->insertItem(Waterfall, tr("Waterfall"));
  renderingModeComboBoxx->insertItem(Kerning_Comparison, tr("Kerning Comparison"));
  renderingModeLabel->setBuddy(renderingModeComboBoxx);

  kerningModeLabel = new QLabel(tr("Kerning Mode"));
  kerningModeLabel->setAlignment(Qt::AlignTop);
  kerningModeComboBoxx = new QComboBoxx;
  kerningModeComboBoxx->insertItem(KERNING_MODE_NONE, tr("No Kerning"));
  kerningModeComboBoxx->insertItem(KERNING_MODE_NORMAL, tr("Normal"));
  kerningModeComboBoxx->insertItem(KERNING_MODE_SMART, tr("Smart"));
  kerningModeLabel->setBuddy(kerningModeComboBoxx);

  kerningDegreeLabel = new QLabel(tr("Kerning Degree"));
  kerningDegreeLabel->setAlignment(Qt::AlignTop);
  kerningDegreeComboBoxx = new QComboBoxx;
  kerningDegreeComboBoxx->insertItem(KERNING_DEGREE_NONE, tr("None"));
  kerningDegreeComboBoxx->insertItem(KERNING_DEGREE_LIGHT, tr("Light"));
  kerningDegreeComboBoxx->insertItem(KERNING_DEGREE_MEDIUM, tr("Medium"));
  kerningDegreeComboBoxx->insertItem(KERNING_DEGREE_TIGHT, tr("Tight"));
  kerningDegreeLabel->setBuddy(kerningDegreeComboBoxx);

  int width;
  // make all labels have the same width
  width = hintingModeLabel->minimumSizeHint().width();
  width = qMax(antiAliasingLabel->minimumSizeHint().width(), width);
  width = qMax(lcdFilterLabel->minimumSizeHint().width(), width);
  width = qMax(renderingModeLabel->minimumSizeHint().width(), width);
  width = qMax(kerningModeLabel->minimumSizeHint().width(), width);
  width = qMax(kerningDegreeLabel->minimumSizeHint().width(), width);
  hintingModeLabel->setMinimumWidth(width);
  antiAliasingLabel->setMinimumWidth(width);
  lcdFilterLabel->setMinimumWidth(width);
  renderingModeLabel->setMinimumWidth(width);
  kerningModeLabel->setMinimumWidth(width);
  kerningDegreeLabel->setMinimumWidth(width);

  // ensure that all items in combo boxes fit completely;
  // also make all combo boxes have the same width
  width = hintingModeComboBoxx->minimumSizeHint().width();
  width = qMax(antiAliasingComboBoxx->minimumSizeHint().width(), width);
  width = qMax(lcdFilterComboBox->minimumSizeHint().width(), width);
  width = qMax(renderingModeComboBoxx->minimumSizeHint().width(), width);
  width = qMax(kerningModeComboBoxx->minimumSizeHint().width(), width);
  width = qMax(kerningDegreeComboBoxx->minimumSizeHint().width(), width);
  hintingModeComboBoxx->setMinimumWidth(width);
  antiAliasingComboBoxx->setMinimumWidth(width);
  lcdFilterComboBox->setMinimumWidth(width);
  renderingModeComboBoxx->setMinimumWidth(width);
  kerningModeComboBoxx->setMinimumWidth(width);
  kerningDegreeComboBoxx->setMinimumWidth(width);

  gammaLabel = new QLabel(tr("Gamma"));
  gammaLabel->setAlignment(Qt::AlignRight);
  gammaSlider = new QSlider(Qt::Horizontal);
  gammaSlider->setRange(0, 30); // in 1/10th
  gammaSlider->setTickPosition(QSlider::TicksBelow);
  gammaSlider->setTickInterval(5);
  gammaLabel->setBuddy(gammaSlider);

  xLabel = new QLabel(tr("Embolden X"));
  yLabel = new QLabel(tr("Embolden Y"));
  xLabel->setAlignment(Qt::AlignRight);
  yLabel->setAlignment(Qt::AlignRight);
  embolden_x_Slider = new QSlider(Qt::Horizontal);
  embolden_y_Slider = new QSlider(Qt::Horizontal);
  embolden_x_Slider->setRange(0, 100); // 5 = 0.005
  embolden_y_Slider->setRange(0, 100);
  embolden_x_Slider->setTickPosition(QSlider::TicksBelow);
  embolden_x_Slider->setTickInterval(5);
  embolden_y_Slider->setTickPosition(QSlider::TicksBelow);
  embolden_y_Slider->setTickInterval(5);
  xLabel->setBuddy(embolden_x_Slider);
  yLabel->setBuddy(embolden_y_Slider);


  slantLabel = new QLabel(tr("Slant"));
  strokeLabel = new QLabel(tr("Stroke"));
  slantLabel->setAlignment(Qt::AlignRight);
  strokeLabel->setAlignment(Qt::AlignRight);
  slant_Slider = new QSlider(Qt::Horizontal);
  stroke_Slider = new QSlider(Qt::Horizontal);
  slant_Slider->setRange(1, 100); // 5 = 0.05
  stroke_Slider->setRange(1, 100);
  slant_Slider->setTickInterval(2);
  stroke_Slider->setTickInterval(5);
  slantLabel->setBuddy(slant_Slider);
  strokeLabel->setBuddy(stroke_Slider);

  showBitmapCheckBox = new QCheckBox(tr("Show Bitmap"));
  showPointsCheckBox = new QCheckBox(tr("Show Points"));
  showPointNumbersCheckBox = new QCheckBox(tr("Show Point Numbers"));
  showOutlinesCheckBox = new QCheckBox(tr("Show Outlines"));

  infoLeftLayout = new QHBoxLayout;
  infoLeftLayout->addWidget(fontFilenameLabel);

  hintingModeLayout = new QHBoxLayout;
  hintingModeLayout->addWidget(hintingModeLabel);
  hintingModeLayout->addWidget(hintingModeComboBoxx);

  horizontalHintingLayout = new QHBoxLayout;
  horizontalHintingLayout->addSpacing(20); // XXX px
  horizontalHintingLayout->addWidget(horizontalHintingCheckBox);

  verticalHintingLayout = new QHBoxLayout;
  verticalHintingLayout->addSpacing(20); // XXX px
  verticalHintingLayout->addWidget(verticalHintingCheckBox);

  blueZoneHintingLayout = new QHBoxLayout;
  blueZoneHintingLayout->addSpacing(20); // XXX px
  blueZoneHintingLayout->addWidget(blueZoneHintingCheckBox);

  segmentDrawingLayout = new QHBoxLayout;
  segmentDrawingLayout->addSpacing(20); // XXX px
  segmentDrawingLayout->addWidget(segmentDrawingCheckBox);

  warpingLayout = new QHBoxLayout;
  warpingLayout->addSpacing(20); // XXX px
  warpingLayout->addWidget(warpingCheckBox);

  antiAliasingLayout = new QHBoxLayout;
  antiAliasingLayout->addWidget(antiAliasingLabel);
  antiAliasingLayout->addWidget(antiAliasingComboBoxx);

  lcdFilterLayout = new QHBoxLayout;
  lcdFilterLayout->addWidget(lcdFilterLabel);
  lcdFilterLayout->addWidget(lcdFilterComboBox);

  gammaLayout = new QHBoxLayout;
  gammaLayout->addWidget(gammaLabel);
  gammaLayout->addWidget(gammaSlider);

  emboldenVertLayout = new QHBoxLayout;
  emboldenVertLayout->addWidget(yLabel);
  emboldenVertLayout->addWidget(embolden_y_Slider);

  emboldenHorzLayout = new QHBoxLayout;
  emboldenHorzLayout->addWidget(xLabel);
  emboldenHorzLayout->addWidget(embolden_x_Slider);

  slantLayout = new QHBoxLayout;
  slantLayout->addWidget(slantLabel);
  slantLayout->addWidget(slant_Slider);

  strokeLayout = new QHBoxLayout;
  strokeLayout->addWidget(strokeLabel);
  strokeLayout->addWidget(stroke_Slider);

  pointNumbersLayout = new QHBoxLayout;
  pointNumbersLayout->addSpacing(20); // XXX px
  pointNumbersLayout->addWidget(showPointNumbersCheckBox);

  generalTabLayout = new QVBoxLayout;
  generalTabLayout->addWidget(hintingCheckBox);
  generalTabLayout->addLayout(hintingModeLayout);
  generalTabLayout->addWidget(autoHintingCheckBox);
  generalTabLayout->addLayout(horizontalHintingLayout);
  generalTabLayout->addLayout(verticalHintingLayout);
  generalTabLayout->addLayout(blueZoneHintingLayout);
  generalTabLayout->addLayout(segmentDrawingLayout);
  generalTabLayout->addLayout(warpingLayout);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addLayout(antiAliasingLayout);
  generalTabLayout->addLayout(lcdFilterLayout);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addLayout(gammaLayout);
  generalTabLayout->addSpacing(20); // XXX px
  generalTabLayout->addStretch(1);
  generalTabLayout->addWidget(showBitmapCheckBox);
  generalTabLayout->addWidget(showPointsCheckBox);
  generalTabLayout->addLayout(pointNumbersLayout);
  generalTabLayout->addWidget(showOutlinesCheckBox);

  renderLayout = new QHBoxLayout;
  renderLayout->addWidget(renderingModeLabel);
  renderLayout->addWidget(renderingModeComboBoxx);

  kerningLayout = new QHBoxLayout;
  kerningLayout->addWidget(kerningModeLabel);
  kerningLayout->addWidget(kerningModeComboBoxx);

  degreeLayout = new QHBoxLayout;
  degreeLayout->addWidget(kerningDegreeLabel);
  degreeLayout->addWidget(kerningDegreeComboBoxx);

  viewTabLayout = new QVBoxLayout;
  viewTabLayout->addLayout(renderLayout);
  viewTabLayout->addLayout(kerningLayout);
  viewTabLayout->addLayout(degreeLayout);
  viewTabLayout->addLayout(emboldenVertLayout);
  viewTabLayout->addLayout(emboldenHorzLayout);
  viewTabLayout->addLayout(slantLayout);
  viewTabLayout->addLayout(strokeLayout);

  generalTabWidget = new QWidget;
  generalTabWidget->setLayout(generalTabLayout);

  mmgxTabWidget = new QWidget;

  // set layout ftview
  viewTabWidget = new QWidget;
  viewTabWidget->setLayout(viewTabLayout);


  tabWidget = new QTabWidget;
  tabWidget->addTab(generalTabWidget, tr("General"));
  tabWidget->addTab(mmgxTabWidget, tr("MM/GX"));
  tabWidget->addTab(viewTabWidget, tr("Ftview"));

  leftLayout = new QVBoxLayout;
  leftLayout->addLayout(infoLeftLayout);
  leftLayout->addWidget(tabWidget);

  // we don't want to expand the left side horizontally;
  // to change the policy we have to use a widget wrapper
  leftWidget = new QWidget;
  leftWidget->setLayout(leftLayout);

  QSizePolicy leftWidgetPolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  leftWidgetPolicy.setHorizontalStretch(0);
  leftWidgetPolicy.setVerticalPolicy(leftWidget->sizePolicy().verticalPolicy());
  leftWidgetPolicy.setHeightForWidth(leftWidget->sizePolicy().hasHeightForWidth());

  leftWidget->setSizePolicy(leftWidgetPolicy);

  // right side
  glyphIndexLabel = new QLabel;
  glyphNameLabel = new QLabel;
  fontNameLabel = new QLabel;

  glyphScene = new QGraphicsScene;
  gridView->setChecked(true);

  currentGlyphBitmapItem = NULL;
  currentGlyphOutlineItem = NULL;
  currentGlyphPointsItem = NULL;
  currentGlyphPointNumbersItem = NULL;
  currentGlyphSegmentItem = NULL;
  currentRenderAllItem = NULL;

  glyphView = new QGraphicsViewx;
  glyphView->setRenderHint(QPainter::Antialiasing, true);
  glyphView->setDragMode(QGraphicsView::ScrollHandDrag);
  glyphView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
  glyphView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  glyphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  glyphView->setScene(glyphScene);

  sizeLabel = new QLabel(tr("Size "));
  sizeLabel->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox = new QDoubleSpinBox;
  sizeDoubleSpinBox->setAlignment(Qt::AlignRight);
  sizeDoubleSpinBox->setDecimals(1);
  sizeDoubleSpinBox->setRange(1, 500);
  sizeLabel->setBuddy(sizeDoubleSpinBox);

  unitsComboBox = new QComboBox;
  unitsComboBox->insertItem(Units_px, "px");
  unitsComboBox->insertItem(Units_pt, "pt");

  dpiLabel = new QLabel(tr("DPI "));
  dpiLabel->setAlignment(Qt::AlignRight);
  dpiSpinBox = new QSpinBox;
  dpiSpinBox->setAlignment(Qt::AlignRight);
  dpiSpinBox->setRange(10, 600);
  dpiLabel->setBuddy(dpiSpinBox);

  toStartButtonx = new QPushButtonx("|<");
  toM1000Buttonx = new QPushButtonx("-1000");
  toM100Buttonx = new QPushButtonx("-100");
  toM10Buttonx = new QPushButtonx("-10");
  toM1Buttonx = new QPushButtonx("-1");
  toP1Buttonx = new QPushButtonx("+1");
  toP10Buttonx = new QPushButtonx("+10");
  toP100Buttonx = new QPushButtonx("+100");
  toP1000Buttonx = new QPushButtonx("+1000");
  toEndButtonx = new QPushButtonx(">|");

  zoomLabel = new QLabel(tr("Zoom Factor"));
  zoomLabel->setAlignment(Qt::AlignRight);
  zoomSpinBox = new QSpinBoxx;
  zoomSpinBox->setAlignment(Qt::AlignRight);
  zoomSpinBox->setRange(1, 1000 - 1000 % 64);
  zoomSpinBox->setKeyboardTracking(false);
  zoomLabel->setBuddy(zoomSpinBox);

  previousFontButton = new QPushButton(tr("Previous Font"));
  nextFontButton = new QPushButton(tr("Next Font"));
  previousFaceButton = new QPushButton(tr("Previous Face"));
  nextFaceButton = new QPushButton(tr("Next Face"));
  previousNamedInstanceButton = new QPushButton(tr("Previous Named Instance"));
  nextNamedInstanceButton = new QPushButton(tr("Next Named Instance"));

  infoRightLayout = new QGridLayout;
  infoRightLayout->addWidget(glyphIndexLabel, 0, 0);
  infoRightLayout->addWidget(glyphNameLabel, 0, 1);
  infoRightLayout->addWidget(fontNameLabel, 0, 2);

  programNavigationLayout = new QHBoxLayout;
  programNavigationLayout->addStretch(2);
  programNavigationLayout->addWidget(gridView);
  programNavigationLayout->addStretch(1);
  programNavigationLayout->addWidget(allGlyphs);
  programNavigationLayout->addStretch(1);
  programNavigationLayout->addWidget(stringView);
  programNavigationLayout->addStretch(1);
  programNavigationLayout->addWidget(multiView);
  programNavigationLayout->addStretch(1);
  programNavigationLayout->addStretch(2);


  navigationLayout = new QHBoxLayout;
  navigationLayout->setSpacing(0);
  navigationLayout->addStretch(1);
  navigationLayout->addWidget(toStartButtonx);
  navigationLayout->addWidget(toM1000Buttonx);
  navigationLayout->addWidget(toM100Buttonx);
  navigationLayout->addWidget(toM10Buttonx);
  navigationLayout->addWidget(toM1Buttonx);
  navigationLayout->addWidget(toP1Buttonx);
  navigationLayout->addWidget(toP10Buttonx);
  navigationLayout->addWidget(toP100Buttonx);
  navigationLayout->addWidget(toP1000Buttonx);
  navigationLayout->addWidget(toEndButtonx);
  navigationLayout->addStretch(1);

  sizeLayout = new QHBoxLayout;
  sizeLayout->addStretch(2);
  sizeLayout->addWidget(sizeLabel);
  sizeLayout->addWidget(sizeDoubleSpinBox);
  sizeLayout->addWidget(unitsComboBox);
  sizeLayout->addStretch(1);
  sizeLayout->addWidget(dpiLabel);
  sizeLayout->addWidget(dpiSpinBox);
  sizeLayout->addStretch(1);
  sizeLayout->addWidget(zoomLabel);
  sizeLayout->addWidget(zoomSpinBox);
  sizeLayout->addStretch(2);

  fontLayout = new QGridLayout;
  fontLayout->setColumnStretch(0, 2);
  fontLayout->addWidget(nextFontButton, 0, 1);
  fontLayout->addWidget(previousFontButton, 1, 1);
  fontLayout->setColumnStretch(2, 1);
  fontLayout->addWidget(nextFaceButton, 0, 3);
  fontLayout->addWidget(previousFaceButton, 1, 3);
  fontLayout->setColumnStretch(4, 1);
  fontLayout->addWidget(nextNamedInstanceButton, 0, 5);
  fontLayout->addWidget(previousNamedInstanceButton, 1, 5);
  fontLayout->setColumnStretch(6, 2);

  rightLayout = new QVBoxLayout;
  rightLayout->addLayout(infoRightLayout);
  rightLayout->addWidget(glyphView);
  rightLayout->addLayout(programNavigationLayout);
  rightLayout->addLayout(navigationLayout);
  rightLayout->addSpacing(10); // XXX px
  rightLayout->addLayout(sizeLayout);
  rightLayout->addSpacing(10); // XXX px
  rightLayout->addLayout(fontLayout);

  // for symmetry with the left side use a widget also
  rightWidget = new QWidget;
  rightWidget->setLayout(rightLayout);

  // the whole thing
  ftinspectLayout = new QHBoxLayout;
  ftinspectLayout->addWidget(leftWidget);
  ftinspectLayout->addWidget(rightWidget);

  ftinspectWidget = new QWidget;
  ftinspectWidget->setLayout(ftinspectLayout);
  setCentralWidget(ftinspectWidget);
  setWindowTitle("ftinspect");
}


void
MainGUI::createConnections()
{
  connect(hintingCheckBox, SIGNAL(clicked()),
          SLOT(checkHinting()));

  connect(hintingModeComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkHintingMode()));
  connect(antiAliasingComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkAntiAliasing()));
  connect(lcdFilterComboBox, SIGNAL(currentIndexChanged(int)),
          SLOT(checkLcdFilter()));
  connect(renderingModeComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkRenderingMode()));
  connect(kerningModeComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkKerningMode()));
  connect(kerningDegreeComboBoxx, SIGNAL(currentIndexChanged(int)),
          SLOT(checkKerningDegree()));

  connect(allGlyphs, SIGNAL(clicked()),
          SLOT(renderAll()));
  connect(gridView, SIGNAL(clicked()),
          SLOT(gridViewRender()));

  connect(autoHintingCheckBox, SIGNAL(clicked()),
          SLOT(checkAutoHinting()));
  connect(horizontalHintingCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(verticalHintingCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(blueZoneHintingCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(warpingCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(segmentDrawingCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showBitmapCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showPointsCheckBox, SIGNAL(clicked()),
          SLOT(checkShowPoints()));
  connect(showPointNumbersCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(showOutlinesCheckBox, SIGNAL(clicked()),
          SLOT(drawGlyph()));
  connect(gammaSlider, SIGNAL(valueChanged(int)),
          SLOT(drawGlyph()));
  connect(embolden_x_Slider, SIGNAL(valueChanged(int)),
          SLOT(renderAll()));
  connect(embolden_y_Slider, SIGNAL(valueChanged(int)),
          SLOT(renderAll()));
  connect(slant_Slider, SIGNAL(valueChanged(int)),
          SLOT(renderAll()));
  connect(stroke_Slider, SIGNAL(valueChanged(int)),
          SLOT(renderAll()));

  connect(sizeDoubleSpinBox, SIGNAL(valueChanged(double)),
          SLOT(drawGlyph()));
  connect(unitsComboBox, SIGNAL(currentIndexChanged(int)),
          SLOT(checkUnits()));
  connect(dpiSpinBox, SIGNAL(valueChanged(int)),
          SLOT(drawGlyph()));

  connect(zoomSpinBox, SIGNAL(valueChanged(int)),
          SLOT(zoom()));

  connect(previousFontButton, SIGNAL(clicked()),
          SLOT(previousFont()));
  connect(nextFontButton, SIGNAL(clicked()),
          SLOT(nextFont()));
  connect(previousFaceButton, SIGNAL(clicked()),
          SLOT(previousFace()));
  connect(nextFaceButton, SIGNAL(clicked()),
          SLOT(nextFace()));
  connect(previousNamedInstanceButton, SIGNAL(clicked()),
          SLOT(previousNamedInstance()));
  connect(nextNamedInstanceButton, SIGNAL(clicked()),
          SLOT(nextNamedInstance()));

  glyphNavigationMapper = new QSignalMapper;
  connect(glyphNavigationMapper, SIGNAL(mapped(int)),
          SLOT(adjustGlyphIndex(int)));

  connect(toStartButtonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM1000Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM100Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM10Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toM1Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP1Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP10Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP100Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toP1000Buttonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));
  connect(toEndButtonx, SIGNAL(clicked()),
          glyphNavigationMapper, SLOT(map()));

  glyphNavigationMapper->setMapping(toStartButtonx, -0x10000);
  glyphNavigationMapper->setMapping(toM1000Buttonx, -1000);
  glyphNavigationMapper->setMapping(toM100Buttonx, -100);
  glyphNavigationMapper->setMapping(toM10Buttonx, -10);
  glyphNavigationMapper->setMapping(toM1Buttonx, -1);
  glyphNavigationMapper->setMapping(toP1Buttonx, 1);
  glyphNavigationMapper->setMapping(toP10Buttonx, 10);
  glyphNavigationMapper->setMapping(toP100Buttonx, 100);
  glyphNavigationMapper->setMapping(toP1000Buttonx, 1000);
  glyphNavigationMapper->setMapping(toEndButtonx, 0x10000);

  connect(fontWatcher, SIGNAL(fileChanged(const QString&)),
          SLOT(watchCurrentFont()));
  connect(timer, SIGNAL(timeout()),
          SLOT(watchCurrentFont()));
}


void
MainGUI::createActions()
{
  loadFontsAct = new QAction(tr("&Load Fonts"), this);
  loadFontsAct->setShortcuts(QKeySequence::Open);
  connect(loadFontsAct, SIGNAL(triggered()), SLOT(loadFonts()));

  closeFontAct = new QAction(tr("&Close Font"), this);
  closeFontAct->setShortcuts(QKeySequence::Close);
  connect(closeFontAct, SIGNAL(triggered()), SLOT(closeFont()));

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcuts(QKeySequence::Quit);
  connect(exitAct, SIGNAL(triggered()), SLOT(close()));

  showFontNameAct = new QAction(tr("&Font Name"), this);
  connect(showFontNameAct, SIGNAL(triggered()), SLOT(showFontName()));

  showFontTypeAct = new QAction(tr("&Font Type"), this);
  connect(showFontTypeAct, SIGNAL(triggered()), SLOT(showFontType()));

  showCharmapsInfoAct = new QAction(tr("&Charmap Info"), this);
  connect(showCharmapsInfoAct, SIGNAL(triggered()), SLOT(showCharmapsInfo()));

  aboutAct = new QAction(tr("&About"), this);
  connect(aboutAct, SIGNAL(triggered()), SLOT(about()));

  aboutQtAct = new QAction(tr("About &Qt"), this);
  connect(aboutQtAct, SIGNAL(triggered()), SLOT(aboutQt()));
}


void
MainGUI::createMenus()
{
  menuFile = menuBar()->addMenu(tr("&File"));
  menuFile->addAction(loadFontsAct);
  menuFile->addAction(closeFontAct);
  menuFile->addAction(exitAct);

  menuInfo = menuBar()->addMenu(tr("&Elements"));
  menuInfo->addAction(showFontNameAct);
  menuInfo->addAction(showFontTypeAct);
  menuInfo->addAction(showCharmapsInfoAct);

  if (fontList.size() <= 0)
  {
    menuInfo->setEnabled(false);
  }

  menuHelp = menuBar()->addMenu(tr("&Help"));
  menuHelp->addAction(aboutAct);
  menuHelp->addAction(aboutQtAct);
}


void
MainGUI::createStatusBar()
{
  statusBar()->showMessage("");
}


void
MainGUI::clearStatusBar()
{
  statusBar()->clearMessage();
  statusBar()->setStyleSheet("");
}


void
MainGUI::setDefaults()
{
  // set up mappings between property values and combo box indices
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_35] = HintingMode_TrueType_v35;
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_38] = HintingMode_TrueType_v38;
  hintingModesTrueTypeHash[TT_INTERPRETER_VERSION_40] = HintingMode_TrueType_v40;

  hintingModesCFFHash[FT_HINTING_FREETYPE] = HintingMode_CFF_FreeType;
  hintingModesCFFHash[FT_HINTING_ADOBE] = HintingMode_CFF_Adobe;

  lcdFilterHash[FT_LCD_FILTER_DEFAULT] = LCDFilter_Default;
  lcdFilterHash[FT_LCD_FILTER_LIGHT] = LCDFilter_Light;
  lcdFilterHash[FT_LCD_FILTER_NONE] = LCDFilter_None;
  lcdFilterHash[FT_LCD_FILTER_LEGACY] = LCDFilter_Legacy;

  // make copies and remove existing elements...
  QHash<int, int> hmTTHash = hintingModesTrueTypeHash;
  if (hmTTHash.contains(engine->ttInterpreterVersionDefault))
    hmTTHash.remove(engine->ttInterpreterVersionDefault);
  if (hmTTHash.contains(engine->ttInterpreterVersionOther))
    hmTTHash.remove(engine->ttInterpreterVersionOther);
  if (hmTTHash.contains(engine->ttInterpreterVersionOther1))
    hmTTHash.remove(engine->ttInterpreterVersionOther1);

  QHash<int, int> hmCFFHash = hintingModesCFFHash;
  if (hmCFFHash.contains(engine->cffHintingEngineDefault))
    hmCFFHash.remove(engine->cffHintingEngineDefault);
  if (hmCFFHash.contains(engine->cffHintingEngineOther))
    hmCFFHash.remove(engine->cffHintingEngineOther);

  // ... to construct a list of always disabled hinting mode combo box items
  hintingModesAlwaysDisabled = hmTTHash.values();
  hintingModesAlwaysDisabled += hmCFFHash.values();

  for (int i = 0; i < hintingModesAlwaysDisabled.size(); i++)
    hintingModeComboBoxx->setItemEnabled(hintingModesAlwaysDisabled[i],
                                         false);

  // the next four values always non-negative
  currentFontIndex = 0;
  currentFaceIndex = 0;
  currentNamedInstanceIndex = 0;
  currentGlyphIndex = 0;

  currentCFFHintingMode
    = hintingModesCFFHash[engine->cffHintingEngineDefault];
  currentTTInterpreterVersion
    = hintingModesTrueTypeHash[engine->ttInterpreterVersionDefault];

  hintingCheckBox->setChecked(true);

  antiAliasingComboBoxx->setCurrentIndex(AntiAliasing_Normal);
  lcdFilterComboBox->setCurrentIndex(LCDFilter_Light);
  renderingModeComboBoxx->setCurrentIndex(0);

  horizontalHintingCheckBox->setChecked(true);
  verticalHintingCheckBox->setChecked(true);
  blueZoneHintingCheckBox->setChecked(true);

  showBitmapCheckBox->setChecked(true);
  showOutlinesCheckBox->setChecked(true);

  gammaSlider->setValue(18); // 1.8
  //embolden_x_Slider->setValue(40);
  //embolden_y_Slider->setValue(40);
  sizeDoubleSpinBox->setValue(20);
  dpiSpinBox->setValue(96);
  zoomSpinBox->setValue(20);

  checkHinting();
  checkHintingMode();
  checkAutoHinting();
  checkAntiAliasing();
  checkLcdFilter();
  checkShowPoints();
  checkUnits();
  checkCurrentFontIndex();
  checkCurrentFaceIndex();
  checkCurrentNamedInstanceIndex();
  adjustGlyphIndex(0);
  zoom();
  gridViewRender();
}


void
MainGUI::readSettings()
{
  QSettings settings;
//  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
//  QSize size = settings.value("size", QSize(400, 400)).toSize();
//  resize(size);
//  move(pos);
}


void
MainGUI::writeSettings()
{
  QSettings settings;
//  settings.setValue("pos", pos());
//  settings.setValue("size", size());
}


// end of maingui.cpp
