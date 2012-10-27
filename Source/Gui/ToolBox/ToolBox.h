#include <Source\Framework\Thread.h>
#include "Export.h"

class CWndToolbox : public CWnd, public CThread, public CExport
{
	enum {
		Width = 200,
		Height = 80
	};

public:
	bool m_bRunning;
	bool m_bFirst;
	bool m_bAdcEnabled;
	int m_nFocus;
	
	CWndToolbox()
	{
		m_bRunning = false;
		m_nFocus = -1;
		m_bFirst = true;
		m_bAdcEnabled = false;
	}

	void Create( CWnd* pParent )
	{
	  CWnd::Create("CWndToolbox", CWnd::WsHidden | CWnd::WsModal, 
			CRect( (BIOS::LCD::LcdWidth-Width)/2 ,
			(BIOS::LCD::LcdHeight-Height)/2,
			(BIOS::LCD::LcdWidth+Width)/2,
			(BIOS::LCD::LcdHeight+Height)/2 ), pParent);
	}
	virtual void OnPaint()
	{
		CRect rcClient(m_rcClient);
		if ( m_bFirst )
		{
			CDesign::Shadow(rcClient, 0xc0ffffff); // aa rr gg bb
			//rcClient.Deflate( 2, 2, 2, 2 );
			//CDesign::Shadow(rcClient, 0x80ffffff); // aa rr gg bb
			m_bFirst = false;
		}

		if ( m_nFocus == -1 )
			m_nFocus = 0;

		#define FOC(n) (m_nFocus==n)?RGB565(ffffff):RGB565(808080)
		PrintBold( m_rcClient.left + 8, m_rcClient.top + 2, FOC(0), RGB565(000000), 
			m_bAdcEnabled ? "\x10 Pause" : "\x10 Resume" );
		PrintBold( m_rcClient.left + 8, m_rcClient.top + 2 + 16, FOC(1), RGB565(000000), "\x10 Save bitmap");
		PrintBold( m_rcClient.left + 8, m_rcClient.top + 2 + 32, FOC(2), RGB565(000000), "\x10 Save waveform (BIN)");
		PrintBold( m_rcClient.left + 8, m_rcClient.top + 2 + 48, FOC(3), RGB565(000000), "\x10 Save waveform (CVS)");

		char str[32];
		BIOS::DBG::sprintf(str, "bat %d%%", BIOS::GetBattery());
		BIOS::LCD::Printf( m_rcClient.left + 8, m_rcClient.top + 2 + 64, RGB565(000000), RGBTRANS, "\x10 Select  \xfe Close");
		PrintBold( m_rcClient.left + 8 + 120, m_rcClient.top + 2, RGB565(ffff00), RGB565(000000), str);
		//BIOS::LCD::Printf( m_rcClient.left + 8 + 130, m_rcClient.top + 8, RGB565(000000), RGBTRANS, str);
	}

	void PrintBold( int x, int y, ui16 clrFront, ui16 clrBorder, PCSTR szLabel )
	{
		BIOS::LCD::Print( x-1, y  , clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x  , y+1, clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x+1, y  , clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x  , y-1, clrBorder, RGBTRANS, szLabel );

		BIOS::LCD::Print( x-1, y-1, clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x-1, y+1, clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x+1, y-1, clrBorder, RGBTRANS, szLabel );
		BIOS::LCD::Print( x+1, y+1, clrBorder, RGBTRANS, szLabel );

		BIOS::LCD::Print( x+0, y+0, clrFront, RGBTRANS, szLabel );
	}

	virtual BOOL IsRunning()
	{
		return m_bRunning;
	}

	virtual void OnKey(ui16 nKey)
	{
		if ( nKey == BIOS::KEY::KeyDown )
		{
			if ( m_nFocus < 3 )
			{
				m_nFocus++;
				Invalidate();
			}
			return;
		}
		if ( nKey == BIOS::KEY::KeyUp )
		{
			if ( m_nFocus > 0 )
			{
				m_nFocus--;
				Invalidate();
			}
			return;
		}
		if ( nKey == BIOS::KEY::KeyEnter )
		{
			m_bRunning = FALSE;
			return;
		}
		if ( nKey == BIOS::KEY::KeyLeft || nKey == BIOS::KEY::KeyRight )
			return;

		m_nFocus = -1;
		m_bRunning = FALSE;
	}

	virtual int GetResult()
	{
		return m_nFocus;
	}

	void DoModal()
	{
		ui16 buffer[Width*Height];
		BIOS::LCD::GetImage( m_rcClient, buffer );

		m_bRunning = true;
		m_bFirst = true;
		m_bAdcEnabled = BIOS::ADC::Enabled();
		BIOS::ADC::Enable(false);

///		if ( MainWnd.m_wndMenuInput.m_wndListTrigger.IsVisible() )
///			MainWnd.m_wndMenuInput.m_wndListTrigger.Invalidate();

		CWnd* pSafeFocus = GetFocus();
		SetFocus();
		ShowWindow( CWnd::SwShow );
		Invalidate();
		while ( IsRunning() )
		{
			Sleep(20);
		}
		ShowWindow( CWnd::SwHide );
		BIOS::LCD::PutImage( m_rcClient, buffer );

		switch ( GetResult() )
		{
		case 0: 
			// Resume / Pause
			m_bAdcEnabled = !m_bAdcEnabled;
			break;
		case 1: 
			// Save bitmap
			SaveScreenshot();
			BIOS::Beep(200);
			break;
		case 2: 
			// Save wave BIN
			SaveBinary();
			BIOS::Beep(200);
			break;
		case 3: 
			// Save wave SVG
			SaveCvs();
			BIOS::Beep(200);
			break;
		case -1: break;
		}

		BIOS::ADC::Enable( m_bAdcEnabled );

///		if ( MainWnd.m_wndMenuInput.m_wndListTrigger.IsVisible() )
///			MainWnd.m_wndMenuInput.m_wndListTrigger.Invalidate();

		pSafeFocus->SetFocus();
	}

};
