#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect_tab_widget.cpp - tab-widget in channel-track-window for setting up
 *                         effects
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <Qt/QtXml>
#include <QtCore/QMap>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QWhatsThis>
#include <QtGui/QColor>

#else

#include <qdom.h>
#include <qmap.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qcolor.h>
#include <qimage.h>

#define addSeparator insertSeparator
#define addMenu insertItem

#endif

#include "rack_plugin.h"
#include "knob.h"
#include "tempo_sync_knob.h"
#include "tooltip.h"
#include "ladspa_2_lmms.h"
#include "ladspa_control_dialog.h"
#include "embed.h"
#include "gui_templates.h"
#include "main_window.h"


rackPlugin::rackPlugin( QWidget * _parent, 
			ladspa_key_t _key, 
			track * _track, 
			engine * _engine, 
			audioPort * _port ) :
	QWidget( _parent, "rackPlugin" ),
	journallingObject( _engine ),
	m_track( _track ),
	m_port( _port ),
	m_contextMenu( NULL ),
	m_key( _key ),
	m_show( TRUE )
{
	m_effect = new ladspaEffect( _key, eng() );
	m_port->getEffects()->appendEffect( m_effect );
	
	m_name = m_effect->getName();
	
	setFixedSize( 210, 60 );

	QPixmap bg = embed::getIconPixmap( "effect_plugin" );
#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), bg );
	setPalette( pal );
#else
	setErasePixmap( bg );
#endif
	
	m_bypass = new ledCheckBox( "", this, tr( "Turn the effect off" ), 
							eng(), m_track );
	connect( m_bypass, SIGNAL( toggled( bool ) ), 
				this, SLOT( bypassed( bool ) ) );
	toolTip::add( m_bypass, tr( "On/Off" ) );
	m_bypass->setChecked( TRUE );
	m_bypass->move( 3, 3 );
#ifdef QT4
		m_bypass->setWhatsThis(
#else
		QWhatsThis::add( m_bypass,
#endif
					tr( 
"Toggles the effect on or off." ) );
	
	m_wetDry = new knob( knobBright_26, this, 
					tr( "Wet/Dry mix" ), eng(), m_track );
	connect( m_wetDry, SIGNAL( valueChanged( float ) ), 
				this, SLOT( setWetDry( float ) ) );
	m_wetDry->setLabel( tr( "W/D" ) );
	m_wetDry->setRange( 0.0f, 1.0f, 0.01f );
	m_wetDry->setInitValue( 1.0f );
	m_wetDry->move( 27, 5 );
	m_wetDry->setHintText( tr( "Wet Level:" ) + " ", "" );
#ifdef QT4
		m_wetDry->setWhatsThis(
#else
		QWhatsThis::add( m_wetDry,
#endif
					tr( 
"The Wet/Dry knob sets the ratio between the input signal and the effect that "
"shows up in the output." ) );

	m_autoQuit = new tempoSyncKnob( knobBright_26, this, tr( "Decay" ),
							eng(), m_track );
	connect( m_autoQuit, SIGNAL( valueChanged( float ) ), 
				this, SLOT( setAutoQuit( float ) ) );
	m_autoQuit->setLabel( tr( "Decay" ) );
	m_autoQuit->setRange( 1.0f, 8000.0f, 100.0f );
	m_autoQuit->setInitValue( 1 );
	m_autoQuit->move( 60, 5 );
	m_autoQuit->setHintText( tr( "Time:" ) + " ", "ms" );
#ifdef QT4
		m_autoQuit->setWhatsThis(
#else
		QWhatsThis::add( m_autoQuit,
#endif
					tr( 
"The Decay knob controls how many buffers of silence must pass before the "
"plugin stops processing.  Smaller values will reduce the CPU overhead but "
"run the risk of clipping the tail on delay effects." ) );

	m_gate = new knob( knobBright_26, this, tr( "Gate" ), eng(), m_track );
	connect( m_wetDry, SIGNAL( valueChanged( float ) ), 
				this, SLOT( setGate( float ) ) );
	m_gate->setLabel( tr( "Gate" ) );
	m_gate->setRange( 0.0f, 1.0f, 0.01f );
	m_gate->setInitValue( 0.0f );
	m_gate->move( 93, 5 );
	m_gate->setHintText( tr( "Gate:" ) + " ", "" );
#ifdef QT4
		m_gate->setWhatsThis(
#else
		QWhatsThis::add( m_gate,
#endif
					tr( 
"The Gate knob controls the signal level that is considered to be 'silence' "
"while deciding when to stop processing signals." ) );

	m_editButton = new QPushButton( this, "Controls" );
	m_editButton->setText( tr( "Controls" ) );
	QFont f = m_editButton->font();
	m_editButton->setFont( pointSize<7>( f ) );
	m_editButton->setGeometry( 140, 14, 50, 20 );
	connect( m_editButton, SIGNAL( clicked() ), 
				this, SLOT( editControls() ) );
		
	QString name = eng()->getLADSPAManager()->getShortName( _key );
	m_label = new QLabel( this );
	m_label->setText( name );
	f = m_label->font();
	f.setBold( TRUE );
	m_label->setFont( pointSize<7>( f ) );
	m_label->setGeometry( 5, 44, 195, 10 );
	
	QPixmap back = QPixmap( bg.convertToImage().copy( 5, 44, 
							195, 10 ) );
#ifdef QT4
	m_label->setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), back );
	m_label->setPalette( pal );
#else
	m_label->setErasePixmap( back );
#endif
	
	m_controlView = new ladspaControlDialog( 
				eng()->getMainWindow()->workspace(), 
				m_effect, eng(), m_track );
	connect( m_controlView, SIGNAL( closed() ),
				this, SLOT( closeEffects() ) );
	m_controlView->hide();
	
	if( m_controlView->getControlCount() == 0 )
	{
		m_editButton->hide();
	}
	
#ifdef QT4
	this->setWhatsThis(
#else
	QWhatsThis::add( this,
#endif
				tr( 
"Effect plugins function as a chained series of effects where the signal will "
"be processed from top to bottom.\n\n"

"The On/Off switch allows you to bypass a given plugin at any point in "
"time.\n\n"

"The Wet/Dry knob controls the balance between the input signal and the "
"effected signal that is the resulting output from the effect.  The input "
"for one stage is the output from the previous stage, so the 'dry' signal "
"for effects lower in the chain contains all of the previous effects.\n\n"

"The Decay knob controls how long the signal will continue to be processed "
"after the notes have been released.  The effect will stop processing signals "
"when the signal has dropped below a given threshold for a given length of "
"time.  This knob sets the 'given length of time'.  Longer times will require "
"more CPU, so this number should be set low for most effects.  It needs to be "
"bumped up for effects that produce lengthy periods of silence, e.g. "
"delays.\n\n"

"The Gate knob controls the 'given threshold' for the effect's auto shutdown.  "
"The clock for the 'given length of time' will begin as soon as the processed "
"signal level drops below the level specified with this knob.\n\n"

"The Controls button opens a dialog for editing the effect's parameters.\n\n"

"Right clicking will bring up a context menu where you can change the order "
"in which the effects are processed or delete an effect altogether." ) );
}



rackPlugin::~rackPlugin()
{
	delete m_effect;
	delete m_controlView;
}



void rackPlugin::editControls( void )
{
	if( m_show)
	{
		m_controlView->show();
		m_controlView->raise();
		m_show = FALSE;
	}
	else
	{
		m_controlView->hide();
		m_show = TRUE;
	}
}




void rackPlugin::bypassed( bool _state )
{
	m_effect->setBypass( !_state );
}




void rackPlugin::setWetDry( float _value )
{
	m_effect->setWetLevel( _value );
}



void rackPlugin::setAutoQuit( float _value )
{
	float samples = eng()->getMixer()->sampleRate() * _value / 1000.0f;
	Uint32 buffers = 1 + ( static_cast<Uint32>( samples ) / 
			eng()->getMixer()->framesPerAudioBuffer() );
	m_effect->setTimeout( buffers );
}



void rackPlugin::setGate( float _value )
{
	m_effect->setGate( _value );
}




void rackPlugin::contextMenuEvent( QContextMenuEvent * )
{
	m_contextMenu = new QMenu( this );
#ifdef QT4
	m_contextMenu->setTitle( m_effect->getName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" + 
				QString( m_effect->getName() ) + "</b></font>",
				this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	m_contextMenu->addAction( caption );
#endif
	m_contextMenu->addAction( embed::getIconPixmap( "arp_up_on" ), 
						tr( "Move &up" ), 
						this, SLOT( moveUp() ) );
	m_contextMenu->addAction( embed::getIconPixmap( "arp_down_on" ), 
						tr( "Move &down" ),
						this, SLOT( moveDown() ) );
	m_contextMenu->addSeparator();
	m_contextMenu->addAction( embed::getIconPixmap( "cancel" ), 
						tr( "&Remove this plugin" ),
						this, SLOT( deletePlugin() ) );
	m_contextMenu->addSeparator();
	m_contextMenu->addAction( embed::getIconPixmap( "help" ), 
						tr( "&Help" ),  
						this, SLOT( displayHelp() ) );
	m_contextMenu->exec( QCursor::pos() );
	if( m_contextMenu != NULL )
	{
		delete m_contextMenu;
	}
}




void rackPlugin::moveUp()
{
	if( m_contextMenu != NULL )
	{
		delete m_contextMenu;
		m_contextMenu = NULL;
	}
	emit( moveUp( this ) );
}




void rackPlugin::moveDown()
{
	if( m_contextMenu != NULL )
	{
		delete m_contextMenu;
		m_contextMenu = NULL;
	}
	emit( moveDown( this ) );
}



void rackPlugin::deletePlugin()
{
	if( m_contextMenu != NULL )
	{
		delete m_contextMenu;
		m_contextMenu = NULL;
	}
	emit( deletePlugin( this ) );
}




void rackPlugin::displayHelp( void )
{
#ifdef QT4
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
#else
	QWhatsThis::display( QWhatsThis::textFor( this ),
					mapToGlobal( rect().bottomRight() ) );
#endif
}




void FASTCALL rackPlugin::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	_this.setAttribute( "on", m_bypass->isChecked() );
	_this.setAttribute( "wet", m_wetDry->value() );
	_this.setAttribute( "autoquit", m_autoQuit->value() );
	_this.setAttribute( "gate", m_gate->value() );
	m_controlView->saveState( _doc, _this );
}




void FASTCALL rackPlugin::loadSettings( const QDomElement & _this )
{
	m_bypass->setChecked( _this.attribute( "on" ).toInt() );
	m_wetDry->setValue( _this.attribute( "wet" ).toFloat() );
	m_autoQuit->setValue( _this.attribute( "autoquit" ).toFloat() );
	m_gate->setValue( _this.attribute( "gate" ).toFloat() );
	
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_controlView->nodeName() == node.nodeName() )
			{
				m_controlView->restoreState( 
							node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}




void rackPlugin::closeEffects( void )
{
	m_controlView->hide();
	m_show = TRUE;
}




#include "rack_plugin.moc"

#endif

#endif
