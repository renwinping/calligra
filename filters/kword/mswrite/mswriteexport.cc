/* $Id$ */

/* This file is part of the KDE project
   Copyright (C) 2002-2003 Clarence Dang <dang@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License Version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License Version 2 for more details.

   You should have received a copy of the GNU Library General Public License
   Version 2 along with this library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qbuffer.h>
#include <qcstring.h>
#include <qfont.h>
#include <qimage.h>
#include <qtextcodec.h>
#include <qvaluelist.h>
#include <qvaluestack.h>

#include <kdebug.h>
#include <kgenericfactory.h>

#include <koFilterChain.h>
#include "qwmf.h"

#include <KWEFStructures.h>
#include <KWEFBaseWorker.h>
#include <KWEFKWordLeader.h>

#include "libmswrite.h"

#include "mswriteexport.h"


class MSWriteExportFactory : KGenericFactory <MSWriteExport, KoFilter>
{
public:
	MSWriteExportFactory () : KGenericFactory <MSWriteExport, KoFilter> ("kwordmswriteexport")
	{
	}
	
protected:
	virtual void setupTranslations (void)
	{
		KGlobal::locale()->insertCatalogue ("kwordmswritefilter");
	}
};

K_EXPORT_COMPONENT_FACTORY (libmswriteexport, MSWriteExportFactory ());


class WRIDevice : public MSWrite::Device
{
private:
	FILE *m_outfp;
	long m_outfp_pos, m_outfp_eof;

public:
	WRIDevice () : m_outfp (NULL), m_outfp_pos (0), m_outfp_eof (0)
	{
	}
	
	~WRIDevice ()
	{
		closeFile ();
	}
	
	bool openFile (const char *fileName)
	{
		m_outfp = fopen (fileName, "wb");
		if (!m_outfp)
		{
			error (MSWrite::Error::FileError, "could not open file for writing\n");
			return false;
		}
		
		return true;
	}
	
	bool closeFile (void)
	{
		if (m_outfp)
		{
			if (fclose (m_outfp))
			{
				error (MSWrite::Error::FileError, "could not close output file\n");
				return false;
			}
			
			m_outfp = NULL;
		}
		
		return true;
	}
	
	bool read (MSWrite::Byte *, const MSWrite::DWord)
	{
		error (MSWrite::Error::InternalError, "reading from an output file?\n");
		return false;
	}
	
	bool write (const MSWrite::Byte *buf, const MSWrite::DWord numBytes)
	{
		size_t cwrite = fwrite (buf, 1, numBytes, m_outfp);
		if (cwrite != numBytes)
		{
			error (MSWrite::Error::FileError, "could not write to output file\n");
			return false;
		}
		
		// keep track of where we are up to in the output file and where EOF is
		m_outfp_pos += numBytes;	
		if (m_outfp_pos > m_outfp_eof)
			m_outfp_eof = m_outfp_pos;
		
		return true;
	}
	
	bool seek (const long offset, const int whence)
	{
		long absloc;
		switch (whence)
		{
		case SEEK_SET:
			absloc = offset;
			break;
		case SEEK_CUR:
			absloc = m_outfp_pos + offset;
			break;
		case SEEK_END:
			absloc = m_outfp_eof + offset;
			break;
		default:
			error (MSWrite::Error::InternalError, "invalid whence passed to WRIDevice::seek\n");
			return false;
		}
		
		if (absloc > m_outfp_eof)
		{
			kdDebug (30509) << "Want to seek to " << absloc
					  				<< " but EOF is at " << m_outfp_eof
									<< "; so writing " << absloc - m_outfp_eof
									<< " zeros" << endl;

			if (fseek (m_outfp, m_outfp_eof, SEEK_SET))
			{
				error (MSWrite::Error::FileError,
							"could not seek to EOF in output file\n");
				return false;
			}
				
			MSWrite::Byte *zero = new MSWrite::Byte [absloc - m_outfp_eof];
			if (!zero)
			{
				error (MSWrite::Error::OutOfMemory,
							"could not allocate memory for zeros\n");
				return false;
			}
			memset (zero, 0, absloc - m_outfp_eof);
			if (!write (zero, absloc - m_outfp_eof)) return false;
			delete [] zero;
			
			m_outfp_eof = absloc;
			m_outfp_pos = absloc;
			return true;
		}
		else
		{
			if (fseek (m_outfp, offset, whence) == 0)
			{
				m_outfp_pos = absloc;
				return true;
			}
			else
			{
				error (MSWrite::Error::FileError, "could not seek output file\n");
				return false;
			}
		}
	}
	
	long tell (void)
	{
		return ftell (m_outfp);
	}
	
	void debug (const char *s)
	{
		kdDebug (30509) << s;
	}
	void debug (const int i)
	{
		kdDebug (30509) << i;
	}
	void error (const int errorCode, const char *message,
					const char * /*file*/ = "", const int /*lineno*/ = 0,
					MSWrite::DWord /*token*/ = MSWrite::Device::NoToken)
	{
		if (errorCode == MSWrite::Error::Warn)
			kdWarning (30509) << message;
		else
		{
			m_error = errorCode;
			kdError (30509) << message;
		}
	}
};


class KWordMSWriteWorker : public KWEFBaseWorker
{
private:
	WRIDevice *m_device;
	MSWrite::InternalGenerator *m_generator;
	
	MSWrite::PageLayout m_pageLayout;
	MSWrite::Word m_pageHeight, m_pageWidth,
						m_topMargin, m_leftMargin, m_bottomMargin, m_rightMargin;
	MSWrite::Word m_pageNumberStart;
						
	// for charset conversion
	QTextCodec *m_codec;
	QTextEncoder *m_encoder;

	QValueList <HeaderData> m_headerData;
	QValueList <FooterData> m_footerData;

	int m_headerType, m_footerType;
	bool m_hasHeader, m_isHeaderOnFirstPage;
	bool m_hasFooter, m_isFooterOnFirstPage;
	
	enum inWhatPossiblities
	{
		Nothing,
		Header,
		Footer,
		Body
	} m_inWhat;

#if 0
	class CounterInfo
	{
	public:
		CounterInfo () : m_upto (0), m_style (CounterData::STYLE_NUM),
								m_leftText (""), m_rightText ("."),
								m_currentText ("0."), m_customCharacter (0),
								m_isIncreasing (true)
		{
		}
		
		int m_upto;
		CounterData::Style m_style;
		QString m_leftText, m_rightText;
		QString m_currentText;
		int m_customCharacter;
		bool m_isIncreasing;	// like not a bullet
		// TODO: custom things
	};
	
	QValueList <CounterInfo> m_listCounterList;
#endif

public:
	KWordMSWriteWorker () : m_device (NULL), m_generator (NULL),
									m_pageHeight (0xFFFF), m_pageWidth (0xFFFF),
									m_topMargin (0xFFFF), m_leftMargin (0xFFFF),
									m_bottomMargin (0xFFFF), m_rightMargin (0xFFFF),
									m_encoder (NULL),
									m_hasHeader (false), m_hasFooter (false),
									m_inWhat (Nothing)
	{
		// just select windows-1252 until the "Select Encoding" dialog works
		m_codec = QTextCodec::codecForName ("CP 1252");

		if (m_codec)
			m_encoder = m_codec->makeEncoder();
		else
			kdWarning (30509) << "Cannot convert to Win Charset!" << endl;
	
		m_device = new WRIDevice;
		if (!m_device)
		{
			kdError (30509) << "Could not allocate memory for Device" << endl;
			return;
		}
		
		m_generator = new MSWrite::InternalGenerator;
		if (!m_generator)
		{
			m_device->error (MSWrite::Error::OutOfMemory, "could not allocate memory for InternalGenerator\n");
			return;
		}
		
		m_generator->setDevice (m_device);
	}

	~KWordMSWriteWorker ()
	{
		delete m_generator;
		delete m_device;
		delete m_encoder;
	}
	
	int getError (void) const
	{
		return m_device->bad ();
	}
	
	bool doOpenFile (const QString &outFileName, const QString &)
	{
		// constructor failed?
		if (!m_device || !m_generator)
			return false;
		
		if (!m_device->openFile (outFileName.latin1 ())) return false;
		
		return true;
	}
	
	bool doCloseFile (void)
	{
		if (!m_device->closeFile ()) return false;
		return true;
	}
	 
	bool doOpenDocument (void)
	{
		kdDebug (30509) << "doOpenDocument ()" << endl;
		
		// We can't open the document here because we don't yet have
		// PageLayout * as doFullPaperFormat() and doFullPaperBorders()
		// haven't been called yet.
		// 
		// doTrulyOpenDocument truly opens the document and is called by
		// doOpenBody()
		
		return true;
	}
	
	bool doTrulyOpenDocument (void)
	{
		// TODO: Write's UI doesn't allow the user to change Height or Width so
		//       setting it here might not be a good idea...
		m_pageLayout.setPageHeight (m_pageHeight);
		m_pageLayout.setPageWidth (m_pageWidth);
		m_pageLayout.setPageNumberStart (m_pageNumberStart);
		m_pageLayout.setTopMargin (m_topMargin);
		m_pageLayout.setLeftMargin (m_leftMargin);
		m_pageLayout.setTextHeight (m_pageHeight - m_topMargin - m_bottomMargin);
		m_pageLayout.setTextWidth (m_pageWidth - m_leftMargin - m_rightMargin);
		
		// TODO: libexport
		// headerFromTop
		// footerFromTop
				
		if (!m_generator->writeDocumentBegin (MSWrite::Format::Write_3_0,
															&m_pageLayout)) return false;
		
		return true;
	}
		
	bool doCloseDocument (void)
	{
		kdDebug (30509) << "doCloseDocument ()" << endl;
		
		if (!m_generator->writeDocumentEnd (MSWrite::Format::Write_3_0,
														&m_pageLayout)) return false;
		
		return true;
	}

	bool doFullPaperFormat (const int format,
						 			const double width, const double height,
									const int orientation)
	{
		kdDebug (30509) << "doFullPaperFormat ("
								<< format << ", "
								<< width << ", "
								<< height << ", "
								<< orientation << ")" << endl;

		// TODO: does "format" or "orientation" matter?
		
		m_pageHeight = MSWrite::Word (Point2Twip (height));
		m_pageWidth = MSWrite::Word (Point2Twip (width));
		
		return true;
	}

	bool doFullPaperBorders (const double top, const double left,
						 				const double bottom, const double right)
	{
		kdDebug (30509) << "doFullPaperBorders ("
								<< top << ", "
								<< left << ", "
								<< bottom << ", "
								<< right << ")" << endl;

		m_topMargin = MSWrite::Word (Point2Twip (top));
		m_leftMargin = MSWrite::Word (Point2Twip (left));
		m_bottomMargin = MSWrite::Word (Point2Twip (bottom));
		m_rightMargin = MSWrite::Word (Point2Twip (right));

		return true;
	}

	bool doVariableSettings (const VariableSettingsData &varSettings)
	{
		m_pageNumberStart = MSWrite::Word (varSettings.startingPageNumber);
		
		kdDebug (30509) << "doVariableSettings pageNumberStart="
								<< m_pageNumberStart << endl;
		return true;
	}

	// In Write the header/footer must be the same for every page except that
	// you can choose to not display it on the first page
	//
	// This filter aims to be as lossless as possible so if we can't
	// accomodate the types of headers/footers found in KWord, we at least
	// print out the paragraphs in the body
	bool doPageInfo (int headerType, int footerType)
	{
		kdDebug (30509) << "doPageInfo (headerType=" << headerType
								<< ", footerType=" << footerType
								<< ")" << endl;

		m_headerType = headerType;
		switch (headerType)
		{
		case 0:	// same on all pages
		case 3:	// different on even and odd pages
			m_isHeaderOnFirstPage = true;
			break;
		case 1:	// different on first, even and odd pages
		case 2:	// different on first and other pages
			m_isHeaderOnFirstPage = false;
			break;
		default:
			kdWarning (30509) << "Unknown headerType: " << headerType << endl;
			m_isHeaderOnFirstPage = false;	// just a guess
			break;
		}
		
		m_footerType = footerType;
		switch (footerType)
		{
		case 0:	// same on all pages
		case 3:	// different on even and odd pages
			m_isFooterOnFirstPage = true;
			break;
		case 1:	// different on first, even and odd pages
		case 2:	// different on first and other pages
			m_isFooterOnFirstPage = false;
			break;
		default:
			kdWarning (30590) << "Unknown footerType: " << footerType << endl;
			m_isFooterOnFirstPage = false;	// just a guess
			break;
		}
		
		return true;
	}

	bool isParaListEmpty (const QValueList <ParaData> &para)
	{
		if (para.count () == 1)
		{
			if (para.first ().text.isEmpty ())
				return true;
		}
		
		return false;
	}
	
	bool doHeader (const HeaderData &header)
	{
		kdDebug (30509) << "doHeader (header.page=" << header.page << ")" << endl;
	
		if (isParaListEmpty (header.para))
		{
			kdDebug (30509) << "\tEmpty, ignoring" << endl;
			return true;
		}

	#if 0	// "don't need, probably don't work"
		switch (m_headerType)
		{
		case 0:	// same on all pages
			if (header.page != HeaderData::PAGE_ALL)
				return true;
			break;
		case 3:	// different on even and odd pages
			if (header.page != HeaderData::PAGE_ODD &&
					header.page != HeaderData::PAGE_EVEN)
				return true;
			break;
		case 1:	// different on first, even and odd pages
			// accept everything
			break;
		case 2:	// different on first and other pages
			if (header.page != HeaderData::PAGE_FIRST &&
					header.page != HeaderData::PAGE_ODD)
				return true;
			break;
		}
	#endif
		
		m_hasHeader = true;
		m_headerData.push_back (header);
		return true;
	}
        
	bool doFooter (const FooterData &footer)
	{
		kdDebug (30509) << "doFooter (footer.page=" << footer.page << ")" << endl;

		if (isParaListEmpty (footer.para))
		{
			kdDebug (30509) << "\tEmpty, ignoring" << endl;
			return true;
		}

	#if 0	// "don't need, probably don't work"
		switch (m_footerType)
		{
		case 0:	// same on all pages
			if (footer.page != FooterData::PAGE_ALL)
				return true;
			break;
		case 3:	// different on even and odd pages
			if (footer.page != FooterData::PAGE_ODD &&
					footer.page != FooterData::PAGE_EVEN)
				return true;
			break;
		case 1:	// different on first, even and odd pages
			// accept everything
			break;
		case 2:	// different on first and other pages
			if (footer.page != FooterData::PAGE_FIRST &&
					footer.page != FooterData::PAGE_ODD)
				return true;
			break;
		}
	#endif
		
		m_hasFooter = true;
		m_footerData.push_back (footer);
		return true;
	}
	
	bool doOpenBody (void)
	{
		kdDebug (30509) << "doOpenBody ()" << endl;
		
		//
		// Document Start
		//
		if (!doTrulyOpenDocument ()) return false;
		
		
		//
		// Footers followed by Headers (in this order)
		//

		bool wroteFooter = false;
		m_inWhat = Footer;
			
		for (QValueList <FooterData>::Iterator it = m_footerData.begin ();
				it != m_footerData.end ();
				it++)
		{
			if ((*it).page != FooterData::PAGE_FIRST)
			{
				if (!wroteFooter)
				{
					if (!m_generator->writeFooterBegin ()) return false;
					wroteFooter = true;
				}

				if (!doFullParagraphList ((*it).para)) return false;
				it = --m_footerData.erase (it);
			}
		}
		if (wroteFooter)
			if (!m_generator->writeFooterEnd ()) return false;

		bool wroteHeader = false;
		m_inWhat = Header;
		
		for (QValueList <HeaderData>::Iterator it = m_headerData.begin ();
			it != m_headerData.end ();
			it++)
		{
			if ((*it).page != HeaderData::PAGE_FIRST)
			{
				if (!wroteHeader)
				{
					if (!m_generator->writeHeaderBegin ()) return false;
					wroteHeader = true;
				}
				
				if (!doFullParagraphList ((*it).para)) return false;
				it = --m_headerData.erase (it);
			}
		}
		if (wroteHeader)	
			if (!m_generator->writeHeaderEnd ()) return false;
		
			
		//
		// Body Start
		//

		m_inWhat = Body;
		if (!m_generator->writeBodyBegin ()) return false;
		// KWord doesn't have a PageTable but we must emit the pageNew
		// signal at least once
		if (!m_generator->writePageNew ()) return false;
		
		// dump remaining header paragraphs at the start of the body
		for (QValueList <HeaderData>::Iterator it = m_headerData.begin ();
				it != m_headerData.end ();
				it++)
		{
			kdDebug (30509) << "BODY START ADDING HEADER: " << (*it).page << endl;
			if (!doFullParagraphList ((*it).para)) return false;
			it = --m_headerData.erase (it);
		}
		
		// dump remaining footer paragraphs too
		for (QValueList <FooterData>::Iterator it = m_footerData.begin ();
				it != m_footerData.end ();
				it++)
		{
			kdDebug (30509) << "BODY START ADDING FOOTER: " << (*it).page << endl;
			if (!doFullParagraphList ((*it).para)) return false;
			it = --m_footerData.erase (it);
		}
	
		return true;
	}
	
	bool doCloseBody (void)
	{
		kdDebug (30509) << "doCloseBody ()" << endl;
		
		if (!m_generator->writeBodyEnd ()) return false;
		
		return true;
	}

	// device that can either read from or write to a QBuffer
	// (but not both at the same time, please :))
	class QBufferDevice : public MSWrite::Device
	{
	private:
		QBuffer *m_buffer;
		
	public:
		QBufferDevice (QBuffer *buffer)
		{
			m_buffer = buffer;
		}
		
		bool read (MSWrite::Byte *buf, const MSWrite::DWord numBytes)
		{
			if (m_buffer->readBlock ((char *) buf, (Q_ULONG) numBytes) != Q_LONG (numBytes))
			{
				error (MSWrite::Error::FileError, "could not read from QBuffer (not really a FileError)\n");
				return false;
			}
			else
				return true;
		}
		
		bool write (const MSWrite::Byte *buf, const MSWrite::DWord numBytes)
		{
			if (m_buffer->writeBlock ((char *) buf, (Q_ULONG) numBytes) != Q_LONG (numBytes))
			{
				error (MSWrite::Error::FileError, "could not write to QBuffer (not really a FileError)\n");
				return false;
			}
			else
				return true;
		}
		
		// normally we must write zeros if we seek past EOF
		// but we know that won't happen :)
      bool seek (const long offset, const int whence)
		{
			long absoffset;
			switch (whence)
			{
			case SEEK_SET:
				absoffset = offset;
				break;
			case SEEK_CUR:
				absoffset = m_buffer->at () + offset;
				break;
			case SEEK_END:
				absoffset = m_buffer->size () + offset;
				break;
			default:
				error (MSWrite::Error::InternalError, "unknown seek\n");
				return false;
			}
			
			if (absoffset > long (m_buffer->size ()))
			{
				error (MSWrite::Error::InternalError, "seek past EOF unimplemented\n");
				return false;
			}
			
			if (!m_buffer->at (absoffset))
			{
				error (MSWrite::Error::FileError, "QBuffer could not seek (not really a FileError)\n");
				return false;
			}
			
			return true;
		}
		
      long tell (void)
		{
			return long (m_buffer->at ());
		}
		
		void debug (const char *s)
		{
			kdDebug (30509) << s;
		}
		void debug (const int i)
		{
			kdDebug (30509) << i;
		}
		
		void error (const int errorCode, const char *message,
						const char * /*file*/ = "", const int /*lineno*/ = 0,
						MSWrite::DWord /*token*/ = MSWrite::Device::NoToken)
		{
			if (errorCode == MSWrite::Error::Warn)
				kdWarning (30509) << message;
			else
			{
				m_error = errorCode;
				kdError (30509) << message;
			}
		}
	};
	
	class WMFRecord : public MSWrite::NeedsDevice
	{
	public:
		static const int s_size = 6;

	protected:
		MSWrite::Byte m_data [s_size];

		MSWrite::DWord m_size;	// record size in Words including everything in this struct
		MSWrite::Word m_function;

		MSWrite::Short m_args [100];	// "ought to be enough for anybody"
		int m_argUpto;

	public:
		WMFRecord () : m_argUpto (0)
		{
		}

		WMFRecord (const MSWrite::DWord size, const MSWrite::Word function, MSWrite::Device *device)
						: MSWrite::NeedsDevice (device),
							m_size (size), m_function (function),
							m_argUpto (0)
		{
		}

		MSWrite::DWord getSize (void) const	{	return m_size;	}
		void setSize (const MSWrite::DWord size)	{	m_size = size;	}

		MSWrite::Word getFunction (void) const	{	return m_function;	}
		void setFunction (const MSWrite::Word function)	{	m_function = function;	}

		void add (const MSWrite::Short arg)
		{
			m_args [m_argUpto++] = arg;
		}

		bool readFromDevice (void)
		{
			if (!m_device->readInternal (m_data, 6)) return false;
			MSWrite::ReadDWord (m_size, m_data + 0);
			MSWrite::ReadWord (m_function, m_data + 4);
			printf ("Size (Words): %i  Size (Bytes): %i  Function: %04X  (func=%02X,numArgs=%i)\n",
						m_size, m_size * sizeof (MSWrite::Word), m_function, m_function & 255, m_function >> 8);
	#if 1
			if (m_function == 0 && m_size == 3)	// last record
				return false;
	#endif

			switch (m_function)
			{
			case 0x0103:
				printf ("\tSetMapMode\n");
				break;
			case 0x020c:
				printf ("\tSetWindowExt\n");
				break;
			case 0x020b:
				printf ("\tSetWindowOrg\n");
				break;
			case 0x0b41:
				printf ("\tDibStretchBlt\n");
				break;
			default:
				printf ("\tUnknown function\n");
			}

			long offset = m_device->tellInternal ();

			for (int i = 0; i < ((m_function == 0x0b41) ? 10 : (m_function >> 8)); i++)
			{
				MSWrite::Byte data [2];
				if (!m_device->readInternal (data, 2)) return false;
				MSWrite::ReadShort (m_args [i], data);
				printf ("\tArg (rev) #%i=%i\n", i, m_args [i]);
			}

	#if 0
			// arguments are reversed normally
			int u = 0;
			for (int i = (m_function >> 8) - 1; i >= 0; i--, u++)
				printf ("\tArg #%i=%u\n", u, m_args [i]);
	#endif

		#if 1
			if (m_function == 0xb41)
			{
				// just curious but what's the infoHeader like?
				MSWrite::BMP_BitmapInfoHeader bih;
				bih.setDevice (m_device);
				if (!bih.readFromDevice ()) return false;
			}
		#endif

			m_size *= sizeof (MSWrite::Word);	// in Bytes now
			m_size -= 6;	// skip past prefix
			printf (">>> At: %li  Next: %li\n", m_device->tellInternal (), offset + m_size);
			if (!m_device->seekInternal (offset + m_size, SEEK_SET)) return false;
			return true;
		}

		bool writeToDevice (void)
		{
			MSWrite::WriteDWord (m_size, m_data + 0);
			MSWrite::WriteWord (m_function, m_data + 4);
			if (!m_device->writeInternal (m_data, 6)) return false;

			for (int i = 0; i < ((m_function == 0x0B41) ? 10/*not 11*/ : (m_function >> 8)); i++)
			{
				MSWrite::Byte data [2];
				MSWrite::WriteShort (m_args [i], data);
				if (!m_device->writeInternal (data, 2)) return false;
			}

			return true;
		}
	};

	// converts a DIB to a Standard WMF
	bool BMP2WMF (MSWrite::Device &readDevice, MSWrite::Device &writeDevice)
	{
		// read BMP's FileHeader
		MSWrite::BMP_BitmapFileHeader bfh;
		bfh.setDevice (&readDevice);
		if (!bfh.readFromDevice ()) return false;

		// WMF bitmap doesn't contain FileHeader
		MSWrite::DWord totalBytes = bfh.getTotalBytes () - MSWrite::BMP_BitmapFileHeader::s_size;

		// read BMP's InfoHeader to get some info...
		MSWrite::BMP_BitmapInfoHeader bih;
		bih.setDevice (&readDevice);
		if (!bih.readFromDevice ()) return false;

		// get some info about the image
		MSWrite::Long width = bih.getWidth ();
		MSWrite::Long height = bih.getHeight ();


		// write WMF Header
		MSWrite::WMFHeader wmfHeader;
		wmfHeader.setDevice (&writeDevice);

		MSWrite::DWord maxRecordSizeBytes
			= (totalBytes
				+ 10 * sizeof (MSWrite::Word)/*parameters for DibStretchBlt*/
				+ WMFRecord::s_size);
		wmfHeader.setFileSize ((MSWrite::WMFHeader::s_size
										+ WMFRecord::s_size + 1 * sizeof (MSWrite::Word)/*SetMapMode*/
										+ WMFRecord::s_size + 2 * sizeof (MSWrite::Word)/*SetWindowExt*/
										+ WMFRecord::s_size + 2 * sizeof (MSWrite::Word)/*SetWindowOrg*/
										+ maxRecordSizeBytes/*DibStretchBlt*/
										+ WMFRecord::s_size/*Sentinel*/)
											/ sizeof (MSWrite::Word));
		wmfHeader.setMaxRecordSize (maxRecordSizeBytes / sizeof (MSWrite::Word));
		if (!wmfHeader.writeToDevice ()) return false;

		WMFRecord wmfRecordSetMapMode (4/*(Words)*/, 0x0103, &writeDevice);
		wmfRecordSetMapMode.add (8/*MM_ANISOTROPIC*/);
		if (!wmfRecordSetMapMode.writeToDevice ()) return false;

		WMFRecord wmfRecordSetWindowExt (5/*(Words)*/, 0x020C, &writeDevice);
		wmfRecordSetWindowExt.add (-height);
		wmfRecordSetWindowExt.add (width);
		if (!wmfRecordSetWindowExt.writeToDevice ()) return false;

		WMFRecord wmfRecordSetWindowOrg (5/*(Words)*/, 0x020B, &writeDevice);
		wmfRecordSetWindowOrg.add (0);
		wmfRecordSetWindowOrg.add (0);
		if (!wmfRecordSetWindowOrg.writeToDevice ()) return false;

		WMFRecord wmfRecordBMP (maxRecordSizeBytes / sizeof (MSWrite::Word),
										0x0B41/*DibStretchBlt*/,
										&writeDevice);
		wmfRecordBMP.add (32);	// ?
		wmfRecordBMP.add (204);	// ?
		wmfRecordBMP.add (height);	// src height
		wmfRecordBMP.add (width);	// src width
		wmfRecordBMP.add (0);	// src y
		wmfRecordBMP.add (0);	// src x
		wmfRecordBMP.add (-height);	// dest height
		wmfRecordBMP.add (width);	// dest width
		wmfRecordBMP.add (0);	// dest y
		wmfRecordBMP.add (0);	// dest x
		if (!wmfRecordBMP.writeToDevice ()) return false;

		// write BMP InfoHeader back to the device
		bih.setDevice (&writeDevice);
		if (!bih.writeToDevice ()) return false;

		long left = totalBytes - MSWrite::BMP_BitmapInfoHeader::s_size;
		while (left)
		{
			MSWrite::Byte data [1024];
			long amountToRead = left > 1024 ? 1024 : left;
			if (!readDevice.readInternal (data, amountToRead)) return false;
			if (!writeDevice.writeInternal (data, amountToRead)) return false;

			left -= amountToRead;
		}

		WMFRecord wmfRecordSentinel (3/*(Words)*/, 0x0000, &writeDevice);
		if (!wmfRecordSentinel.writeToDevice ()) return false;

	#if 1
		// bug with Word97?
		MSWrite::Byte zero = 0;
		if (!writeDevice.writeInternal (&zero, sizeof (MSWrite::Byte))) return false;
	#endif

		return true;
	}

	// all windows measurements depend on there being 72 dots/points per inch
	static double getDimen72DPI (const int measurement, const int dotsPerMeter)
	{
		kdDebug (30509) << "getDimen72DPI (measurement=" << measurement
								<< ",dotsPerMeter=" << dotsPerMeter << ")" << endl;

		// Can't get resolution?
		// Assume that we are already 72dpi
		if (dotsPerMeter <= 0)
			return double (measurement);
		
		// 2834.65 = 100 / 2.54 * 72
		return double (measurement) * 2834.65 / double (dotsPerMeter);
	}

	//
	// Note: if we suffer from a conversion error in this function (and can't
	// export the image), we _still_ return true, not false because there is
	// nothing worse than a filter that aborts due to its own incompetence [1]
	// (don't flame the author please, just blame the function :)).  But if we
	// do suffer from a file-like error, we abort right away because something
	// bad (memory corruption, internal error...) is happening!
	//
	// Why then _do_ we abort on text errors when images tell a thousand
	// words?  Because this saying is wrong and text is probably more
	// important to the user.
	//
	// [1] yes, "worse than an itch you can't scratch"
	//
	bool processImage (const FrameAnchor &frameAnchor,
								const MSWrite::FormatParaProperty *paraPropIn,
								const MSWrite::FormatCharProperty *charPropIn)
	{
		kdDebug (30509) << "processImage()" << endl;
		

		// Write supports images in 3 formats:
		//
		// 1. Monochrome BMP (not very useful)
		// 2. OLE (hard to work with and only supported in ver >= 3.1)
		// 3. Standard WMF
		//
		// So we convert all images to WMF for convenience.
		// We don't even bother saving Monochrome BMPs "as is" because
		// there's no point (just save it in WMF to make life easier)
		// 
		// But a Standard WMF is basically a BMP with some headers/GDI calls
		// so the conversion process is like this:
		//
		// start->WMF->finish
		// start->BMP->WMF->finish
		// start->???->BMP->WMF->finish
		// 
		
		double imageActualWidth = -1, imageActualHeight = -1;
		MSWrite::DWord imageSize = 0;

		QString imageType;
		int pos = frameAnchor.picture.koStoreName.findRev ('.');
		if (pos != -1) imageType = frameAnchor.picture.koStoreName.mid (pos).lower ();
		kdDebug (30509) << "imageType: " << imageType << endl;
		
		QByteArray imageData;
		kdDebug (30509) << "Reading image: " << frameAnchor.picture.koStoreName << endl;
		if (!loadSubFile (frameAnchor.picture.koStoreName, imageData))
			ErrorAndQuit (MSWrite::Error::FileError, "could not open image from store\n");
			
		// FSM
		for (;;)
		{
			if (imageType == ".wmf")
			{
				imageSize = imageData.size ();
				if (imageActualWidth == -1 && imageActualHeight == -1)
				{
					QBuffer buffer (imageData);
					buffer.open (IO_ReadOnly);

					// load WMF
					QWinMetaFile wmf;
					if (!wmf.load (buffer))
					{
						kdError (30509) << "Could not open WMF - Invalid Format!" << endl;
						return true;
					}

					QRect dimen = wmf.bbox ();
					// TODO: if you look in the QWmf code, it actually reads the
					// width/height from the PlaceableHeader (if possible),
					// then parses the records for the width/height again.
					// The trouble is that the PlaceableHeader contains
					// measurements in twips and the records are in points...
					// TODO: what about the inch/dpi field? Or is it irrelevant
					// because of above (records like setWindowExt store 72dpi
					// points)
					imageActualWidth = Point2Twip (abs (dimen.width ()));
					imageActualHeight = Point2Twip (abs (dimen.height ()));

					if (wmf.isPlaceable ())
					{
						kdDebug (30509) << "Converting Placeable WMF" << endl;

						// Remove Aldus Placeable WMF Header
						for (int i = 0; i < int (imageSize) - 22; i++)
							imageData [i] = imageData [i + 22];
						
						imageData.resize (imageSize - 22);
						imageSize -= 22;
					}
					else if (wmf.isEnhanced ())
					{
						kdDebug (30509) << "Enhanced WMF unsupported by QWmf" << endl;
						
						return true;
					}
					// Standard WMF
					else
					{
						kdDebug (30509) << "Standard WMF - no conversion required" << endl;
					}
				}
				
				kdDebug (30509) << "Now WMF: width=" << imageActualWidth
										<< " height=" << imageActualHeight
										<< " size=" << imageSize
										<< endl;

				// we're done!
				break;
			}
			// TODO: DDB?
			else if (imageType == ".bmp")
			{
				QImage image (imageData);
				if (image.isNull ())
				{
					kdError (30509) << "QImage IsNull: Line=" << __LINE__ << endl;
					return true;
				}
					
				// TODO: Standard WMF can't take 32-bit?
				if (image.depth () == 32)
				{
					kdWarning (30509) << "32bpp BMP unsupported" << endl;
#if 0
					QImage newImage = image.convertDepth (16/*24 not supported by QT*/);
					if (newImage.isNull ())
						ErrorAndQuit (MSWrite::Error::InternalError, "could not convert 32-bit BMP to 16-bit\n");
						
					QImageIO imageIO (newImage);
					if (!imageIO.read ())
						return false;
					
					QBuffer outBuffer (imageData);
					outBuffer.open (IO_WriteOnly);
					imageIO.setIODevice (&outBuffer);
					imageIO.setFormat ("BMP");
					if (!imageIO.write ())
						return false;
					
					outBuffer.close ();
					
					// reload image
					if (!image.loadFromData (imageData))
						ErrorAndQuit (MSWrite::Error::InternalError, "could not convert 32-bit BMP to 16-bit (2)\n");
#endif
					//return true;
				}
				
				if (imageActualWidth == -1 && imageActualHeight == -1)
				{
					imageActualWidth = Point2Twip (getDimen72DPI (image.width (), image.dotsPerMeterX ()));
					imageActualHeight = Point2Twip (getDimen72DPI (image.height (), image.dotsPerMeterY ()));
				}

				kdDebug (30509) << "Now BMP: width=" << imageActualWidth
										<< " height=" << imageActualHeight
										<< " size=" << imageSize
										<< endl;
				
				QByteArray imageWMF;
					// input device
					QBuffer inBuffer (imageData);
					inBuffer.open (IO_ReadOnly);
					QBufferDevice inDevice (&inBuffer);
					
					// output device
					QBuffer outBuffer (imageWMF);
					outBuffer.open (IO_WriteOnly);
					QBufferDevice outDevice (&outBuffer);

					// BMP --> WMF
					if (!BMP2WMF (inDevice, outDevice))
					{
						kdError (30509) << "BMP to WMF conversion error" << endl;
						return true;
					}

					outBuffer.close ();
					inBuffer.close ();
				imageData = imageWMF.copy ();
				
				imageType = ".wmf";
			}
			else
			{
				if (imageActualWidth == -1 && imageActualHeight == -1)
				{
					QImage image (imageData);
					if (image.isNull())
					{
						kdError (30509) << "QImage isNull: Line=" << __LINE__ << endl;
						return true;
					}

					imageActualWidth = Point2Twip (getDimen72DPI (image.width (), image.dotsPerMeterX ()));
					imageActualHeight = Point2Twip (getDimen72DPI (image.height (), image.dotsPerMeterY ()));
				}
				
				kdDebug (30509) << "Foreign format: width=" << imageActualWidth
										<< " height=" << imageActualHeight
										<< " size=" << imageSize
										<< endl;

				QByteArray imageBMP;
					// input device
					QBuffer inBuffer (imageData);
					inBuffer.open (IO_ReadOnly);
				
					// read foreign image
					QImageIO imageIO (&inBuffer, NULL);
					if (!imageIO.read ())
					{
						kdError (30509) << "Could not read foreign format" << endl;
						return true;
					}
					
					// output device
					QBuffer outBuffer (imageBMP);
					outBuffer.open (IO_WriteOnly);
					
					// write BMP
					imageIO.setIODevice (&outBuffer);
					imageIO.setFormat ("BMP");
					if (!imageIO.write ())
					{
						kdError (30509) << "Could not convert to BMP" << endl;
						return true;
					}
					
					outBuffer.close ();
					inBuffer.close ();
				imageData = imageBMP.copy ();
			
				imageType = ".bmp";
			}
		}
		

		kdDebug (30509) << "Actual dimensions: width=" << imageActualWidth
								<< " height=" << imageActualHeight << endl;

		kdDebug (30509) << "KOffice position: left=" << frameAnchor.left
								<< " right=" << frameAnchor.right
								<< " top=" << frameAnchor.top
								<< " bottom=" << frameAnchor.bottom
								<< endl;
		
		kdDebug (30509) << "Indent=" << MSWrite::Word (Point2Twip (frameAnchor.left)) - m_leftMargin << endl;
		
		double displayedWidth = Point2Twip (frameAnchor.right - frameAnchor.left);
		double displayedHeight = Point2Twip (frameAnchor.bottom - frameAnchor.top);
		
		kdDebug (30509) << "displayedWidth=" << displayedWidth
								<< " displayedHeight=" << displayedHeight
								<< endl;


		//
		// Start writing out the image now 
		//
		// Note: here, we can start returning false again because the errors
		// won't be conversion-related
		//

		MSWrite::Image image;
		image.setIsWMF (true);
		if (paraPropIn->getAlignment () != MSWrite::Alignment::Centre)
			image.setIndent (MSWrite::Word (Point2Twip (frameAnchor.left)) - m_leftMargin);
		else
		{
			// TODO: what is the image offset relative to (it's not always rel. to the left margin)?
			kdDebug (30509) << "\tCentred paragraph, cannot position image" << endl;
		}
		
		image.setOriginalWidth (imageActualWidth);
		image.setOriginalHeight (imageActualHeight);
		image.setDisplayedWidth (displayedWidth);
		image.setDisplayedHeight (displayedHeight);
		image.setExternalImageSize (imageSize);

		MSWrite::FormatParaProperty paraProp; paraProp = *paraPropIn;
		paraProp.setIsObject (true);
		paraProp.setLeftIndent (0);	// not neccessary but...
		if (!m_generator->writeParaInfoBegin (&paraProp, NULL, &image))
			return false;

		// yes, images have character formatting as well
		// (character formatting _must_ cover entire document)
		// (but I think it's ignored)
		MSWrite::FormatCharProperty charProp; charProp = *charPropIn;
		if (!m_generator->writeCharInfoBegin (&charProp))
			return false;

		// actually write image
		if (!m_generator->writeBinary ((const MSWrite::Byte *) (const char *) imageData.data (), imageSize)) return false;

		// 2nd argument endOfParagraph is ignored by InternalGenerator so
		// there's no need to specify it
		if (!m_generator->writeCharInfoEnd (&charProp, true))
			return false;
;
		if (!m_generator->writeParaInfoEnd (&paraProp, NULL, &image))
			return false;


		kdDebug (30509) << "processImage() successful!" << endl;
		return true;
	}
	
	bool processTable (const Table &table)
	{
		// just dump the table out for now (no layout)
		for (QValueList <TableCell>::ConstIterator it = table.cellList.begin ();
				it != table.cellList.end ();
				it++)
		{
			if (!doFullParagraphList (*(*it).paraList)) return false;
		}
		
		return true;
	}

#if 0
	// stolen directly from lib/kotext/koparagcounter.cc	
	QString getRomanNumber (const int n)
	{
		const QCString RNUnits[] = {"", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
		const QCString RNTens[] = {"", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
		const QCString RNHundreds[] = {"", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};
		const QCString RNThousands[] = {"", "m", "mm", "mmm"};

		return QString::fromLatin1( RNThousands[ ( n / 1000 ) ] +
												RNHundreds[ ( n / 100 ) % 10 ] +
												RNTens[ ( n / 10 ) % 10 ] +
												RNUnits[ ( n ) % 10 ] );
	}
#endif

	bool processCounter (const CounterData &counter)
	{
		kdDebug (30509) << "processCounter(counter.text=" << counter.text << ")" << endl;

		if (!counter.text.isEmpty ())
		{
			// isn't this wonderful? :)
			if (!processText (counter.text)) return false;
			if (!processText (" ")) return false;
		}

		return true;

	#if 0	
		if (counter.numbering != CounterData::NUM_LIST &&
				counter.numbering != CounterData::NUM_CHAPTER)
		{
			if (counter.numbering == CounterData::NUM_NONE)
			{
				m_listCounterList.clear ();
			}
			else
			{
				kdWarning (30509) << "Unknown counter.numbering: "
										<< counter.numbering << endl;
			}
			
			return true;
		}

		bool setValues = false;
		int currentDepth = m_listCounterList.count () - 1;
		
		kdDebug (30509) << "counter.depth=" << counter.depth
								<< " currentDepth=" << currentDepth << endl;
		if (counter.depth < currentDepth)
		{
			kdDebug (30509) << "depth<current, therefore deleting after" << endl;
			
			for (QValueList <CounterInfo>::Iterator it
					= m_listCounterList.at (counter.depth + 1);
					it != m_listCounterList.end ();
					it = m_listCounterList.erase (it))
				;
			setValues = true;
		}
		else if (counter.depth == currentDepth &&
					(counter.style != m_listCounterList.last ().m_style ||
						counter.lefttext != m_listCounterList.last ().m_leftText ||
						counter.righttext != m_listCounterList.last ().m_rightText ||
						counter.customCharacter != m_listCounterList.last().m_customCharacter))
		{
			kdDebug (30509) << "new list of the same depth but different look" << endl;
			setValues = true;
		}
		else if (counter.depth > currentDepth)
		{
			kdDebug (30509) << "new list of more depth: "
									<< counter.depth - currentDepth << endl;
			
			for (int i = 0; i < counter.depth - currentDepth; i++)
			{
				CounterInfo newOne;
				m_listCounterList.push_back (newOne);
			}
			setValues = true;
		}
		
		CounterInfo &ci = *m_listCounterList.at (counter.depth);

		// in a new list?
		if (setValues)
		{
			if (counter.start > 0)
				ci.m_upto = counter.start;
			else
				ci.m_upto = 1;
			ci.m_style = counter.style;
			ci.m_leftText = counter.lefttext;
			ci.m_rightText = counter.righttext;
			ci.m_customCharacter = counter.customCharacter;

			// TODO customFont
			
			kdDebug (30509) << "new list: upto=" << ci.m_upto << " "
									<< "style=" << ci.m_style << " "
									<< "leftText=" << ci.m_leftText << " "
									<< "rightText=" << ci.m_rightText << " "
									<< "customChar=" << ci.m_customCharacter << endl;
		}
		

		// is this list increasing?
		// prefix text
		QString nextText = ci.m_leftText;
		
		switch (ci.m_style)
		{
		default:
			kdWarning (30509) << "Unknown listStyle: " << ci.m_style << endl;
		case CounterData::STYLE_NONE:
			ci.m_isIncreasing = false;
			break;
		case CounterData::STYLE_NUM:
		{
			nextText += QString::number (ci.m_upto);
			ci.m_upto++;
			ci.m_isIncreasing = true;
			break;
		}
		case CounterData::STYLE_ALPHAB_L:
		{
			QString cache;
			for (int i = ci.m_upto; i > 0; i /= 26)	// backwards
				cache = QChar ('a' - 1 + (i % 26)) + cache;
			nextText += cache;
			ci.m_upto++;
			ci.m_isIncreasing = true;
			break;
		}	
		case CounterData::STYLE_ALPHAB_U:
		{
			QString cache;
			for (int i = ci.m_upto; i > 0; i /= 26)	// backwards
				cache = QChar ('A' - 1 + (i % 26)) + cache;
			nextText += cache;
			ci.m_upto++;
			ci.m_isIncreasing = true;
			break;
		}
		case CounterData::STYLE_ROM_NUM_L:
		{
			nextText += getRomanNumber (ci.m_upto);
			ci.m_upto++;
			ci.m_isIncreasing = true;
			break;
		}
		case CounterData::STYLE_ROM_NUM_U:
		{
			nextText += getRomanNumber (ci.m_upto).upper ();
			ci.m_upto++;
			ci.m_isIncreasing = true;
			break;
		}
		case CounterData::STYLE_CUSTOMBULLET:
		{
			nextText += counter.customCharacter;
			ci.m_isIncreasing = false;
			break;
		}
		case CounterData::STYLE_CUSTOM:
		{
			nextText += '-';
			ci.m_isIncreasing = false;
			// TODO
			break;
		}
		// hollow circle
		case CounterData::STYLE_CIRCLEBULLET:
		{
			nextText += 'o';
			ci.m_isIncreasing = false;
			break;
		}
		// filled square
		case CounterData::STYLE_SQUAREBULLET:
		{
			nextText += '#';
			ci.m_isIncreasing = false;
			break;
		}	
		// filled circle
		case CounterData::STYLE_DISCBULLET:
		{
			nextText += '*';
			ci.m_isIncreasing = false;
			break;
		}
		// hollow square
		case CounterData::STYLE_BOXBULLET:
		{
			nextText += "[]";	// not a single character but shouldn't cause probs
			ci.m_isIncreasing = false;
			break;
		}}
		
		// suffix text
		nextText += ci.m_rightText;
		
		if (ci.m_isIncreasing)
		{
			if (counter.depth > 0)
				ci.m_currentText = (*m_listCounterList.at (counter.depth - 1)).m_currentText + nextText;
			else
				ci.m_currentText = nextText;
		}
		else
		{
			ci.m_currentText = nextText;
			for (int i = 0; i < int (m_listCounterList.count ()) - 1; i++)
			{
					ci.m_currentText += " ";
			}
		}
			
		QString output = ci.m_currentText;
		output += " ";
		
		// finally output the counter
		if (!processText (output)) return false;
		return true;
	#endif
	}
	
	void processFormatData (MSWrite::FormatCharProperty &charProp,
						 			const TextFormatting &f)
	{
		if (!f.fontName.isEmpty ())
		{
			// create new Font with Name
			MSWrite::Font font ((const MSWrite::Byte *) (const char *) f.fontName.utf8 ());
			kdDebug (30509) << "FontName " << f.fontName << endl;
			
			// get Font Family
			QFont QTFontInfo (f.fontName);
			switch (QTFontInfo.styleHint ())
			{
			case QFont::Serif:
				kdDebug (30509) << "FontFamily Serif" << endl;
				font.setFamily (MSWrite::Font::Roman);
				break;
			case QFont::SansSerif:
				kdDebug (30509) << "FontFamily SansSerif" << endl;
				font.setFamily (MSWrite::Font::Swiss);
				break;
			case QFont::Courier:
				kdDebug (30509) << "FontFamily Courier" << endl;
				font.setFamily (MSWrite::Font::Modern);
				break;
			case QFont::OldEnglish:
				kdDebug (30509) << "FontFamily OldEnglish" << endl;
				font.setFamily (MSWrite::Font::Decorative);
				break;
			default:
				kdDebug (30509) << "FontFamily DontKnow" << endl;
				// it's either DontCare or MSWrite::Font::Script
				font.setFamily (MSWrite::Font::DontCare);
				break;
			}
			
			charProp.setFont (&font);
		}
		if (f.fontSize > 0) charProp.setFontSize (f.fontSize);

		charProp.setIsItalic (f.italic);
		charProp.setIsUnderlined (f.underline);	// TODO: underlineWord
		charProp.setIsBold (f.weight > (50/*normal*/ + 75/*bold*/) / 2);
        
		switch (f.verticalAlignment)
		{
		case 0:	// normal
			charProp.setIsNormalPosition ();
			break;
		case 1:	// subscript
			charProp.setIsSubscript ();
			break;
		case 2:	// superscript
			charProp.setIsSuperscript ();
			break;
		}

		// TODO: fontAttribute;
	}
	
	static MSWrite::Word getClosestLineSpacing (const double points)
	{
		const double twips = Point2Twip (points);
		
	#if 1
		if (twips < double ((MSWrite::LineSpacing::Single + MSWrite::LineSpacing::OneAndAHalf) / 2))
			return MSWrite::LineSpacing::Single;
		else if (twips < double ((MSWrite::LineSpacing::OneAndAHalf + MSWrite::LineSpacing::Double) / 2))
			return MSWrite::LineSpacing::OneAndAHalf;
		else
			return MSWrite::LineSpacing::Double;
	#else	// or do we want a non-"standard" linespacing?
		return MSWrite::Word (twips);
	#endif
	}
	
	bool doFullParagraphList (const QValueList <ParaData> &paraList)
	{
		for (QValueList <ParaData>::ConstIterator it = paraList.begin ();
				it != paraList.end ();
				it ++)
		{
			if (!doFullParagraph (*it)) return false;
		}
	
		return true;
	}
	
	bool doFullParagraph (const ParaData &paraData)
	{
		return doFullParagraph (paraData.text,
							 			paraData.layout,
										paraData.formattingList);
	}
	
	bool doFullParagraph (const QString &paraText,
						 			const LayoutData &layout,
									const ValueListFormatData &paraFormatDataList)
	{
		MSWrite::FormatParaProperty paraProp;
		
		if (m_inWhat == Body)
			paraProp.setIsNormalParagraph (true);
		else
		{
			if (m_inWhat == Header)
			{
				paraProp.setIsHeader (true);
				paraProp.setIsOnFirstPage (m_isHeaderOnFirstPage);
			}
			else if (m_inWhat == Footer)
			{
				paraProp.setIsFooter (true);
				paraProp.setIsOnFirstPage (m_isFooterOnFirstPage);
			}
		}
		
		paraProp.setIsText (true);
		
		// Alignment
		if (!layout.alignment.isEmpty ())
		{
			if (layout.alignment == "left")
				// quite useless since MSWrite::Alignment::Left is the default anyway
				paraProp.setAlignment (MSWrite::Alignment::Left);
			else if (layout.alignment == "right")
				paraProp.setAlignment (MSWrite::Alignment::Right);
			else if (layout.alignment == "center")
				paraProp.setAlignment (MSWrite::Alignment::Center);
			else if (layout.alignment == "justify")
				paraProp.setAlignment (MSWrite::Alignment::Justify);
			else
				kdWarning (30509) << "Unknown Alignment: " << layout.alignment << endl;
		}
		
		// Indentation
		if (layout.indentFirst) paraProp.setLeftIndentFirstLine (MSWrite::Short (Point2Twip (layout.indentFirst)));
		if (layout.indentLeft >= 0) paraProp.setLeftIndent (MSWrite::Word (Point2Twip (layout.indentLeft)));
		if (layout.indentRight >= 0) paraProp.setRightIndent (MSWrite::Word (Point2Twip (layout.indentRight)));
	#if 1
		kdDebug (30509) << "Indent: " << Point2Twip (layout.indentFirst) << " "
												<< Point2Twip (layout.indentLeft) << " "
												<< Point2Twip (layout.indentRight) << endl;
	#endif
		
		// Line Spacing
		MSWrite::Word lineSpacing = MSWrite::LineSpacing::Normal;
		switch (layout.lineSpacingType)
		{
		case LayoutData::LS_SINGLE:
			lineSpacing = MSWrite::LineSpacing::Normal;
			break;
		case LayoutData::LS_ONEANDHALF:
			lineSpacing = MSWrite::LineSpacing::OneAndAHalf;
			break;
		case LayoutData::LS_DOUBLE:
			lineSpacing = MSWrite::LineSpacing::Double;
			break;
		case LayoutData::LS_CUSTOM:
		case LayoutData::LS_ATLEAST:
			lineSpacing = getClosestLineSpacing (layout.lineSpacing);
			break;
		case LayoutData::LS_MULTIPLE:
			break;
		default:
			kdWarning (30509) << "unknown lineSpacingType \'" << layout.lineSpacingType << "\'" << endl;
		}
		paraProp.setLineSpacing (lineSpacing);

		// Tabs are a Document Property, not a Paragraph Property, in Write, yet are stored for each paragraph.
		// It seems that Write applies the 1st paragraph's Tabulator settings to the _entire_ document
		// Word97 and KWord, however, will treat them like a Paragraph Property
		int numTabs = 0;
		for (TabulatorList::ConstIterator tabIt = layout.tabulatorList.begin ();
				tabIt != layout.tabulatorList.end ();
				tabIt++)
		{
			MSWrite::FormatParaPropertyTabulator tab;
			
			// Write's UI only supports 12 as opposed to the 14 supposedly
			// supported in the file so let's play it safe and quit when
			// we reach 12
			// Actually, KWord's UI also only supports 12 so this should never be true
			if (numTabs >= 12)
			{
				kdWarning (30509) << "Write does not support more 12 tabulators, not writing out all tabulators" << endl;
				break;
			}

			if ((*tabIt).m_type == 3)
				tab.setIsDecimal ();
			else
				// Write only supports Decimal and Left tabs
				tab.setIsNormal ();
			
			tab.setIndent (MSWrite::Word (Point2Twip ((*tabIt).m_ptpos)));

			// int m_filling;
			// double m_width;
			if ((*tabIt).m_filling != TabulatorData::TF_NONE)
				kdWarning (30509) <<  "Write does not support Tabulator Filling" << endl;
			
			paraProp.addTabulator (&tab);
			numTabs++;
		}
		
		// TODO: double      marginTop;      // space before the paragraph  (a negative value means invalid)
		// TODO: double      marginBottom;   // space after the paragraph (a negative value means invalid)

		// TODO: QString     styleName;
		// TODO: QString     styleFollowing;
		
		if (!m_generator->writeParaInfoBegin (&paraProp)) return false;
	
		// get this paragraph's "default formatting"
		MSWrite::FormatCharProperty charPropDefault;
		processFormatData (charPropDefault, layout.formatData.text);
		
		MSWrite::DWord uptoByte = 0;	// relative to start of paragraph
		MSWrite::DWord numBytes = paraText.length ();	// relative to start of paragraph
		
		// empty paragraph
		if (numBytes == 0)
		{
			kdDebug (30509) << "Outputting empty paragraph!" << endl;
			
			// write default character property start
			if (!m_generator->writeCharInfoBegin (&charPropDefault)) return false;
			
			// page break at start of paragraph?
			if (layout.pageBreakBefore)
				if (!m_generator->writePageBreak ()) return false;
				
			// counter data
			processCounter (layout.counter);
				
			// end of line
			if (!m_generator->writeCarriageReturn ()) return false;
			if (!m_generator->writeNewLine (true/*end of paragraph*/)) return false;

			// page break at end of paragraph?
			if (layout.pageBreakAfter)
				if (!m_generator->writePageBreak ()) return false;

			// write default character property end
			if (!m_generator->writeCharInfoEnd (&charPropDefault, true)) return false;
		}
		else
		{
			for (ValueListFormatData::ConstIterator formatIt = paraFormatDataList.begin ();
					formatIt != paraFormatDataList.end ();
					formatIt++)
			{
				bool textSegment = true;
				
				// apply local <FORMAT> tag on top of "default formatting"
				MSWrite::FormatCharProperty charProp; charProp = charPropDefault;
				processFormatData (charProp, (*formatIt).text);

				if (!m_generator->writeCharInfoBegin (&charProp)) return false;

				if (uptoByte == 0)
				{
					// page break at start of paragraph?
					if (layout.pageBreakBefore)
						if (!m_generator->writePageBreak ()) return false;
						
					// counter data
					processCounter (layout.counter);
				}

				// yes, this is slightly premature but it doesn't matter
				uptoByte += (*formatIt).len;

				switch ((*formatIt).id)
				{
				case 0:	// none?
					ErrorAndQuit (MSWrite::Error::InternalError, "Format ID = 0\n");
				case 1:	// text
					if (!processText (paraText.mid ((*formatIt).pos, (*formatIt).len)))
											/*uptoByte == numBytes))*/
							return false;
					break;
				case 2:	// picture (deprecated)
					m_device->error (MSWrite::Error::Warn, "Picture (deprecated) unsupported\n");
					break;
				case 3:	// tabulator (deprecated)
					m_device->error (MSWrite::Error::Warn, "Tabulator (deprecated) unsupported\n");
					break;
				case 4:	// variable
				{
					bool justPrintText = true;

					// Page Number / Number of Pages
					if ((*formatIt).variable.m_type == 4 &&
							(*formatIt).variable.isPageNumber () &&
							m_inWhat != Body/*Write replaces it with '*' in the body*/)
					{
						if (!m_generator->writeCharInfoEnd (&charProp)) return false;
						charProp.setIsPageNumber (true);
						// if you don't do this it will print out the character literally (char 1)
						if (!m_generator->writeCharInfoBegin (&charProp)) return false;
						
						// only variable Write supports (and only in headers/footers)
						// if you don't write out char 1, Write will not treat it as a
						// variable anchor and will print the character out literally
						if (!m_generator->writePageNumber ()) return false;
						
						if (!m_generator->writeCharInfoEnd (&charProp)) return false;
						charProp.setIsPageNumber (false);
						if (!m_generator->writeCharInfoBegin (&charProp)) return false;

						justPrintText = false;
					}

					if (justPrintText)
					{
						if (!processText ((*formatIt).variable.m_text))
							return false;
					}

					break;
				}
				case 5:	// footnote (KOffice 1.1)
					m_device->error (MSWrite::Error::Warn, "Footnote unsupported\n");
					break;
				case 6:	// anchor for frame
					if ((*formatIt).frameAnchor.type == 6)
					{
						kdDebug (30509) << "Table detected" << endl;
						processTable ((*formatIt).frameAnchor.table);
					}
					else if ((*formatIt).frameAnchor.type == 2)
					{
						kdDebug (30509) << "Image detected" << endl;

						// In Write, text cannot be flowed around images so
						// an image occupies an entire paragraph.  So we
						// must end our current paragraph here and recontinue
						// after an "image paragraph"
						if (!m_generator->writeCharInfoEnd (&charProp)) return false;
						if (!m_generator->writeParaInfoEnd (&paraProp)) return false;

						// this will make its own paragraph...
						if (!processImage ((*formatIt).frameAnchor, &paraProp, &charProp)) return false;

						// recontinue paragraph
						if (!m_generator->writeParaInfoBegin (&paraProp)) return false;
						if (!m_generator->writeCharInfoBegin (&charProp)) return false;
					}
					else
						kdWarning (30509) << "Unknown type of anchor: " << (*formatIt).frameAnchor.type << endl;
							
					textSegment = false;
					break;
				}

				if (uptoByte == numBytes)
				{
					if (textSegment)
					{
						// end of line
						if (!m_generator->writeCarriageReturn ()) return false;
						if (!m_generator->writeNewLine (true/*end of paragraph*/)) return false;
					}

					// page break at end of paragraph?
					if (layout.pageBreakAfter)
						if (!m_generator->writePageBreak ()) return false;
				}

				if (!m_generator->writeCharInfoEnd (&charProp, uptoByte == numBytes)) return false;
			}
		}
		
		if (!m_generator->writeParaInfoEnd (&paraProp)) return false;
		kdDebug (30509) << "Just Output " << uptoByte << "/" << numBytes << " with text \'" << paraText.utf8 () << "\'" << endl;

		return true;
	}
	
	template <class dtype>
	dtype min (const dtype a, const dtype b, const dtype c)
	{
		if (a < b && a < c) return a;
		if (b < a && b < c) return b;
		return c;
	}

	#ifndef NDEBUG
		#define DEBUG_PROCESS_TEXT
	#endif
	bool processText (const QString &stringUnicode)
	{
		//
		// Look out for characters in the string and emit signals as appropriate:
		//
		// 1  pageNumber (already taken care of as a variable)
		// 10 newLine
		// 13 carriageReturn (TODO: Oh no!  Can't happen!)
		// 12 pageBreak (TODO: we are in real trouble: this can actually happen without forcing a new paragraph!)
		// 31 optionalHyphen
		// ?  text
		//
		
		int softHyphen = -2, nonBreakingSpace = -2, newLine = -2;
		
		int upto = 0;
		int stringUnicodeLength = stringUnicode.length ();
		while (upto < stringUnicodeLength)
		{
			//
			// look for KWord's special characters as defined in the DTD
			//

			if (softHyphen == -2)
			{
				softHyphen = stringUnicode.find (QChar (0xAD), upto);
				if (softHyphen == -1) softHyphen = INT_MAX;
			}
			
			if (nonBreakingSpace == -2)
			{
				nonBreakingSpace = stringUnicode.find (QChar (0xA0), upto);
				if (nonBreakingSpace == -1) nonBreakingSpace = INT_MAX;
			}
			
			if (newLine == -2)
			{
				newLine = stringUnicode.find (QChar ('\n'), upto);
				if (newLine == -1) newLine = INT_MAX;
			}
				
			// look for the closest one
			int specialLocation = min (softHyphen, nonBreakingSpace, newLine);

			// get substring (either to the end of the original string or before
			// the next closest special character, if any)
			int length = stringUnicodeLength - upto;
			if (specialLocation != INT_MAX)
				length = specialLocation - upto;
			QString substring = stringUnicode.mid (upto, length);

		#ifdef DEBUG_PROCESS_TEXT
			kdDebug (30509) << "Parent string:  upto=" << upto
									<< ",length=" << stringUnicode.length () << endl;
			kdDebug (30509) << "Child string:   length=" << length
									<< " (specialLoc=" << specialLocation << ")" << endl;
		#endif
		
			//
			// convert substring to windows-1251
			//
			
			QCString stringWin;

			// there is a codec, therefore there is an encoder...
			if (m_codec)
			{
				int len;	// don't overwrite length, we need it later

				// convert from Unicode (UTF8)
				stringWin = m_encoder->fromUnicode (substring, len = length);
			}
			else
			{
				// output a plain string still in wrong Character Set
				// (hopefully the user won't notice)
				stringWin = substring.utf8 ();
			}

			
			// output encoded text
			if (!m_generator->writeText ((const MSWrite::Byte *) (const char *) stringWin))
				return false;

			upto += length;
			
			// special character?
			if (specialLocation != INT_MAX)
			{
			#ifdef DEBUG_PROCESS_TEXT
				kdDebug (30509) << "Found special character!" << endl;
			#endif
			
				// output special character
				if (specialLocation == softHyphen)
				{
				#ifdef DEBUG_PROCESS_TEXT
					kdDebug (30509) << "\tSoft Hyphen" << endl;
				#endif
					if (!m_generator->writeOptionalHyphen ()) return false;
					softHyphen = -2;
				}
				else if (specialLocation == nonBreakingSpace)
				{
				#ifdef DEBUG_PROCESS_TEXT
					kdDebug (30509) << "\tNon-breaking Space" << endl;
				#endif
					// don't think Write supports nonBreakingSpace
					if (!m_generator->writeText ((const MSWrite::Byte *) " ")) return false;
					nonBreakingSpace = -2;
				}
				else if (specialLocation == newLine)
				{
				#ifdef DEBUG_PROCESS_TEXT
					kdDebug (30509) << "\tNew Line" << endl;
				#endif
					// \r\n, not just \n
					if (!m_generator->writeCarriageReturn ()) return false;
					if (!m_generator->writeNewLine (true/*InternalGenerator doesn't care*/)) return false;
					newLine = -2;
				}
				else
				{
					ErrorAndQuit (MSWrite::Error::InternalError, "simply impossible specialLocation\n");
				}
				
				// skip past special character
				upto++;
			}
		}	// while (upto < stringUnicodeLength)	{

		return true;
	}
};


MSWriteExport::MSWriteExport (KoFilter *, const char *, const QStringList &)
					: KoFilter()
{
}

MSWriteExport::~MSWriteExport ()
{
}

KoFilter::ConversionStatus MSWriteExport::convert (const QCString &from, const QCString &to)
{
	kdDebug (30509) << "MSWriteExport $Date$ using LibMSWrite "
			  				<< MSWrite::Version << endl;
	
	if (to != "application/x-mswrite" || from != "application/x-kword")
	{
		kdError (30509) << "Internal error!  Filter not implemented?" << endl;
		return KoFilter::NotImplemented;
	}

	KWordMSWriteWorker *worker = new KWordMSWriteWorker;
	if (!worker)
	{
		kdError (30509) << "Could not allocate memory for worker" << endl;
		return KoFilter::OutOfMemory;
	}

	KWEFKWordLeader *leader = new KWEFKWordLeader (worker);
	if (!leader)
	{
		kdError (30509) << "Could not allocate memory for leader" << endl;
		delete worker;
		return KoFilter::OutOfMemory;
	}

	KoFilter::ConversionStatus ret = leader->convert (m_chain, from, to);
	int errorCode = worker->getError ();

	delete leader;
	delete worker;

	// try to return somewhat more meaningful errors than KoFilter::StupidError
	// for the day that KOffice actually reports them to the user properly
	switch (errorCode)
	{
	case MSWrite::Error::Ok:
		kdDebug (30509) << "Returning error code " << ret << endl;
		return ret;	// not KoFilter::OK in case KWEFKWordLeader wants to report something
		
	case MSWrite::Error::Warn:
		kdDebug (30509) << "Error::Warn" << endl;
		return KoFilter::InternalError;	// warnings should _never_ set m_error
		
	case MSWrite::Error::InvalidFormat:
		kdDebug (30509) << "Error::InvalidFormat" << endl;
		return KoFilter::InternalError;	// how can the file I'm _writing_ be of an invalid format?
		
	case MSWrite::Error::OutOfMemory:
		kdDebug (30509) << "Error::OutOfMemory" << endl;
		return KoFilter::OutOfMemory;
		
	case MSWrite::Error::InternalError:
		kdDebug (30509) << "Error::InternalError" << endl;
		return KoFilter::InternalError;
		
	case MSWrite::Error::Unsupported:
		kdDebug (30509) << "Error::Unsupported" << endl;
		return KoFilter::InternalError;
	
	case MSWrite::Error::FileError:
		kdDebug (30509) << "Error::FileError" << endl;
		return KoFilter::CreationError;
	}

	kdWarning (30509) << "Unknown error" << endl;
	return KoFilter::StupidError;
}

#include <mswriteexport.moc>
