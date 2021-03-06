/*
 *  General Matroska Muxer Interface
 *
 *  Copyright (C) 2003-2004, Jory Stone <jcsston @ toughguy.net>
 *  Copyright (C) 2003, John Cannon <spyder482 @ yahoo.com>
 *  Copyright (C) 2002, Steve Lhomme <steve.lhomme @ free.fr>
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 */

/*!
	\version $Id: MatroskaMuxer.h,v 1.1 2004/03/24 05:46:51 jcsston Exp $
	\author Jory Stone           <jcsston @ toughguy.net>
 	\brief

 	\par CVS Log
  	$Log: MatroskaMuxer.h,v $
  	Revision 1.1  2004/03/24 05:46:51  jcsston
  	Initial Commit

  	Revision 1.2  2004/01/04 04:16:01  jcsston
  	It actual works!
  	
  	Revision 1.1  2004/01/04 01:07:31  jcsston
  	Finally some matroska muxing code
  	

*/

#ifndef _MATROSKA_MUXER_H_
#define _MATROSKA_MUXER_H_

#include <ctime>
#include <vector>
#include <deque>
#include <queue>
#include <exception>

#include "mkvwriter/MatroskaAttachment.h"
#include "mkvwriter/MatroskaSimpleTags.h"

// libebml includes
#include "ebml/IOCallback.h"
#include "ebml/StdIOCallback.h"
#include "ebml/EbmlTypes.h"
#include "ebml/EbmlHead.h"
#include "ebml/EbmlVoid.h"
#include "ebml/EbmlCrc32.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlBinary.h"
#include "ebml/EbmlString.h"
#include "ebml/EbmlUnicodeString.h"
#include "ebml/EbmlContexts.h"
#include "ebml/EbmlVersion.h"

// libmatroska includes
#include "matroska/KaxConfig.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxBlockData.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxInfo.h"
#include "matroska/KaxInfoData.h"
#include "matroska/KaxTags.h"
#include "matroska/KaxTag.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxClusterData.h"
#include "matroska/KaxTrackAudio.h"
#include "matroska/KaxTrackVideo.h"
#include "matroska/KaxCues.h"
#include "matroska/KaxCuesData.h"
#include "matroska/KaxAttachments.h"
#include "matroska/KaxAttached.h"
#include "matroska/KaxVersion.h"


namespace mkvwriter {


// Our default timecode scale
static const uint64 DefaultTimecodeScale = 1000000;

class FileIOCallback;

class KaxTimecodeReferenceBlock : public libmatroska::KaxReferenceBlock {
public:
   void SetTimecode(int64 refTimecode) { SetReferencedTimecode(refTimecode); };
};



class MatroskaMuxer {
public:
	MatroskaMuxer();
	~MatroskaMuxer();
	
	int Set_OutputFilename(const std::string &ouputFilename);
	int Set_WritingApp(const UTFstring &writingApp);
	int Set_Lacing(bool bUseLacing);
	int Set_WriteDuration(bool bWriteDuration);
	/// Set the limits of the clusters
	/// \param maxKBSize The number of KBs to limit clusters at
	/// \param maxMSLength The number of milliseconds to limit clusters
	int Set_ClusterLimits(uint32 maxKBSize = 1000, uint32 maxMSLength = 1000);
	/// This sets the global timecode scale,
  /// We divide any timecode you feed us with this to get the timecode in ms
	int Set_TimecodeScale(uint64 timecodeScale);
	int Set_Track_Type(uint8 trackNo, int trackType);
	int Set_Track_CodecID(uint8 trackNo, const std::string &codecID);
	int Set_Track_CodecPrivate(uint8 trackNo, const binary *codecPrivate, uint32 codecPrivateSize);
	int Set_Track_DefaultDuration(uint8 trackNo, uint64 defaultDuration);
	int Set_Track_Language(uint8 trackNo, const std::string &lang);
	int Set_Track_Video(uint8 trackNo, uint32 width, uint32 height, uint32 displayWidth = 0, uint32 displayHeight = 0);
	int Set_Track_Audio(uint8 trackNo, uint8 channels, double sampleRate, uint16 bitdepth = 0, double outputSampleRate = 0);
	int Set_Track_Lacing(uint8 trackNo, bool bUseLacing);
	/// Set's a tag, these are not written until the very end
	/// So you can call this method in the middle muxing, but not after CloseFile()
	/// \param trackNo The track this tag should go with, use 0 if this is to be a global tag
	/// \param name UTF-8 tag name
	/// \param value UTF-8 tag value
	int Set_Tag(uint8 trackNo, const char *name, const char *value);
	
	/// Appends this to the list of attachments.
	/// \param attachment Properties of the attachment; Muxer takes ownership.
	void Attach_File(MatroskaAttachment *attachment);
	
	int Get_Track_CodecID(uint8 trackNo, std::string &codecID);
	
	bool WriteHeaders();

	/// Add a frame
	/// \param trackNo The track this frame belongs to
	/// \param timecode A timecode, it should be in ms
	/// \param duration The duration of this frame, it should also be in ms
	/// \param data The frame data
	/// \param dataSize The size of the frame data in bytes
	/// \param reference1 The first reference this frame has, to flag that it should simply reference the previous frame in this track use REFERENCE_PREV_FRAME
	/// \param reference2 The second reference this frame has
	int AddFrame(uint16 trackNo, uint64 timecode, uint32 duration, const void *data, uint32 dataSize, int32 reference1 = 0, int32 reference2 = 0);

	/// Flush the internal queue and writes the closing headers
	/// After this call the muxer will need to be reset for use again
	int CloseFile();

	uint32 framesMuxed;
	uint32 clustersMuxed;
	enum Preset_References {
		REFERENCE_PREV_FRAME = 2147483647
	};
protected:
	/// Internal add frame method
	int _AddFrame(uint16 trackNo, uint64 timecode, uint32 duration, libmatroska::SimpleDataBuffer *data, std::vector<int32> *references = NULL);
	int _ProcessQueue();
	int _FlushQueue();
	bool _WriteAttachments();
	bool _WriteTags();
	bool _ChangeClusters(uint64 timecode, bool bForce = false);
	int _UpdateDuration(double newDuration);	
	libmatroska::KaxTrackEntry *_LookupTrackNo(uint8 trackNo);

	// Output file/filename
	FileIOCallback *m_outputFile;
	std::string MatroskaFilename;
	UTFstring m_WritingApp;

	
	struct MatroskaFrame {
		MatroskaFrame() {
			timecode = 0;
			duration = 0;
			buffer = NULL;
		};
		int64 timecode;
		int32 duration;
		libmatroska::SimpleDataBuffer *buffer;
		std::vector<int32> references;
	};
	class MatroskaFrameQueue : public std::queue<MatroskaFrame *> {
	public:
		uint16 trackNo;
	};
	std::vector<MatroskaFrameQueue> m_Queue;

	// Matroska File Elements
	libmatroska::KaxSegment FileSegment;
	EbmlVoid *MyDummy;
	libmatroska::KaxSeekHead *MySeekHead;
	libmatroska::KaxSeekHead *MySeekHeadOverFill;
	libmatroska::KaxTracks *m_Tracks;
	libmatroska::KaxCluster *clust;
	libmatroska::KaxBlockGroup *m_MyKaxBlockGroup;
	libmatroska::KaxCues *m_AllCues;
	uint16 m_prevTrack;
	uint64 m_prevTimecode;
	uint64 m_lastTimecode;
	uint64 m_timecodeScale;

	MatroskaTagVector m_Tags;
	MatroskaAttachmentVector m_Attachments;

	bool m_WriteDuration;
	bool m_UseLacing;
	uint8 m_LaceCount;
	uint32 m_maxTimePerCluster;
	uint32 m_maxBytesPerCluster;

	uint32 currentClusterSize;
	uint64 currentClusterBaseTimecode;
};


} // namespace mkvwriter


#endif // _MATROSKA_MUXER_H_

