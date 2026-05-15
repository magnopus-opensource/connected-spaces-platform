/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CSP/Common/MimeTypeHelper.h"

#include <cctype>
#include <memory>

namespace csp::common
{

MimeTypeHelper& MimeTypeHelper::Get()
{
    static MimeTypeHelper instance;
    return instance;
}

const String& MimeTypeHelper::GetMimeType(const String& filePath)
{
    auto segments = filePath.Split('.');
    auto extension = segments[segments.Size() - 1].Trim();
    auto length = extension.Length();
    auto chars = extension.c_str();

    auto lowerChars = std::make_unique<char[]>(length);

    for (size_t i = 0; i < length; ++i)
    {
        lowerChars[i] = static_cast<char>(std::tolower(chars[i]));
    }

    String lowerExtension(lowerChars.get(), length);

    if (m_extensionToMimeTypeMap.HasKey(lowerExtension))
    {
        return m_extensionToMimeTypeMap[lowerExtension];
    }
    else
    {
        return m_default;
    }
}

MimeTypes::MimeTypes() { }

FileExtensions::FileExtensions() { }

MimeTypeHelper::MimeTypeHelper()
{
    m_extensionToMimeTypeMap[FileExtension._3DM] = MimeType.X_WORLD_X_3DMF;
    m_extensionToMimeTypeMap[FileExtension._3DMF] = MimeType.X_WORLD_X_3DMF;
    m_extensionToMimeTypeMap[FileExtension.A] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.AAB] = MimeType.APPLICATION_X_AUTHORWARE_BIN;
    m_extensionToMimeTypeMap[FileExtension.AAM] = MimeType.APPLICATION_X_AUTHORWARE_MAPZ;
    m_extensionToMimeTypeMap[FileExtension.AAS] = MimeType.APPLICATION_X_AUTHORWARE_SEG;
    m_extensionToMimeTypeMap[FileExtension.ABC] = MimeType.TEXT_VND_ABC;
    m_extensionToMimeTypeMap[FileExtension.ACGI] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.AFL] = MimeType.VIDEO_ANIMAFLEX;
    m_extensionToMimeTypeMap[FileExtension.AI] = MimeType.APPLICATION_POSTSCRIPT;
    m_extensionToMimeTypeMap[FileExtension.AIF] = MimeType.AUDIO_AIFF;
    m_extensionToMimeTypeMap[FileExtension.AIFC] = MimeType.AUDIO_AIFF;
    m_extensionToMimeTypeMap[FileExtension.AIFF] = MimeType.AUDIO_AIFF;
    m_extensionToMimeTypeMap[FileExtension.AIM] = MimeType.APPLICATION_X_AIM;
    m_extensionToMimeTypeMap[FileExtension.AIP] = MimeType.TEXT_X_AUDIOSOFT_INTRA;
    m_extensionToMimeTypeMap[FileExtension.ANI] = MimeType.APPLICATION_X_NAVI_ANIMATION;
    m_extensionToMimeTypeMap[FileExtension.AOS] = MimeType.APPLICATION_X_NOKIA_9000_COMMUNICATOR_ADD_ON_SOFTWARE;
    m_extensionToMimeTypeMap[FileExtension.APS] = MimeType.APPLICATION_MIME;
    m_extensionToMimeTypeMap[FileExtension.ARC] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.ARJ] = MimeType.APPLICATION_ARJ;
    m_extensionToMimeTypeMap[FileExtension.ART] = MimeType.IMAGE_X_JG;
    m_extensionToMimeTypeMap[FileExtension.ASF] = MimeType.VIDEO_X_MS_ASF;
    m_extensionToMimeTypeMap[FileExtension.ASM] = MimeType.TEXT_X_ASM;
    m_extensionToMimeTypeMap[FileExtension.ASP] = MimeType.TEXT_ASP;
    m_extensionToMimeTypeMap[FileExtension.ASX] = MimeType.APPLICATION_X_MPLAYER2;
    m_extensionToMimeTypeMap[FileExtension.AU] = MimeType.AUDIO_BASIC;
    m_extensionToMimeTypeMap[FileExtension.AVI] = MimeType.VIDEO_AVI;
    m_extensionToMimeTypeMap[FileExtension.AVS] = MimeType.VIDEO_AVS_VIDEO;
    m_extensionToMimeTypeMap[FileExtension.BCPIO] = MimeType.APPLICATION_X_BCPIO;
    m_extensionToMimeTypeMap[FileExtension.BIN] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.BM] = MimeType.IMAGE_BMP;
    m_extensionToMimeTypeMap[FileExtension.BMP] = MimeType.IMAGE_BMP;
    m_extensionToMimeTypeMap[FileExtension.BOO] = MimeType.APPLICATION_BOOK;
    m_extensionToMimeTypeMap[FileExtension.BOOK] = MimeType.APPLICATION_BOOK;
    m_extensionToMimeTypeMap[FileExtension.BOZ] = MimeType.APPLICATION_X_BZIP2;
    m_extensionToMimeTypeMap[FileExtension.BSH] = MimeType.APPLICATION_X_BSH;
    m_extensionToMimeTypeMap[FileExtension.BZ] = MimeType.APPLICATION_X_BZIP;
    m_extensionToMimeTypeMap[FileExtension.BZ2] = MimeType.APPLICATION_X_BZIP2;
    m_extensionToMimeTypeMap[FileExtension.C] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.C_PLUS_PLUS] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.CAT] = MimeType.APPLICATION_VND_MS_PKI_SECCAT;
    m_extensionToMimeTypeMap[FileExtension.CC] = MimeType.TEXT_X_C;
    m_extensionToMimeTypeMap[FileExtension.CCAD] = MimeType.APPLICATION_CLARISCAD;
    m_extensionToMimeTypeMap[FileExtension.CCO] = MimeType.APPLICATION_X_COCOA;
    m_extensionToMimeTypeMap[FileExtension.CDF] = MimeType.APPLICATION_CDF;
    m_extensionToMimeTypeMap[FileExtension.CER] = MimeType.APPLICATION_PKIX_CERT;
    m_extensionToMimeTypeMap[FileExtension.CHA] = MimeType.APPLICATION_X_CHAT;
    m_extensionToMimeTypeMap[FileExtension.CHAT] = MimeType.APPLICATION_X_CHAT;
    m_extensionToMimeTypeMap[FileExtension.CLASS] = MimeType.APPLICATION_JAVA;
    m_extensionToMimeTypeMap[FileExtension.COM] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.CONF] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.CPIO] = MimeType.APPLICATION_X_CPIO;
    m_extensionToMimeTypeMap[FileExtension.CPP] = MimeType.TEXT_X_C;
    m_extensionToMimeTypeMap[FileExtension.CPT] = MimeType.APPLICATION_X_CPT;
    m_extensionToMimeTypeMap[FileExtension.CRL] = MimeType.APPLICATION_PKCS_CRL;
    m_extensionToMimeTypeMap[FileExtension.CRT] = MimeType.APPLICATION_PKIX_CERT;
    m_extensionToMimeTypeMap[FileExtension.CSH] = MimeType.APPLICATION_X_CSH;
    m_extensionToMimeTypeMap[FileExtension.CSS] = MimeType.APPLICATION_X_POINTPLUS;
    m_extensionToMimeTypeMap[FileExtension.CXX] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.DCR] = MimeType.APPLICATION_X_DIRECTOR;
    m_extensionToMimeTypeMap[FileExtension.DEEPV] = MimeType.APPLICATION_X_DEEPV;
    m_extensionToMimeTypeMap[FileExtension.DEF] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.DER] = MimeType.APPLICATION_X_X509_CA_CERT;
    m_extensionToMimeTypeMap[FileExtension.DIF] = MimeType.VIDEO_X_DV;
    m_extensionToMimeTypeMap[FileExtension.DIR] = MimeType.APPLICATION_X_DIRECTOR;
    m_extensionToMimeTypeMap[FileExtension.DL] = MimeType.VIDEO_DL;
    m_extensionToMimeTypeMap[FileExtension.DOC] = MimeType.APPLICATION_MSWORD;
    m_extensionToMimeTypeMap[FileExtension.DOT] = MimeType.APPLICATION_MSWORD;
    m_extensionToMimeTypeMap[FileExtension.DP] = MimeType.APPLICATION_COMMONGROUND;
    m_extensionToMimeTypeMap[FileExtension.DRW] = MimeType.APPLICATION_DRAFTING;
    m_extensionToMimeTypeMap[FileExtension.DUMP] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.DV] = MimeType.VIDEO_X_DV;
    m_extensionToMimeTypeMap[FileExtension.DVI] = MimeType.APPLICATION_X_DVI;
    m_extensionToMimeTypeMap[FileExtension.DWF] = MimeType.MODEL_VND_DWF;
    m_extensionToMimeTypeMap[FileExtension.DWG] = MimeType.APPLICATION_ACAD;
    m_extensionToMimeTypeMap[FileExtension.DXF] = MimeType.APPLICATION_DXF;
    m_extensionToMimeTypeMap[FileExtension.EL] = MimeType.TEXT_X_SCRIPT_ELISP;
    m_extensionToMimeTypeMap[FileExtension.ELC] = MimeType.APPLICATION_X_ELC;
    m_extensionToMimeTypeMap[FileExtension.ENV] = MimeType.APPLICATION_X_ENVOY;
    m_extensionToMimeTypeMap[FileExtension.EPS] = MimeType.APPLICATION_POSTSCRIPT;
    m_extensionToMimeTypeMap[FileExtension.ES] = MimeType.APPLICATION_X_ESREHBER;
    m_extensionToMimeTypeMap[FileExtension.ETX] = MimeType.TEXT_X_SETEXT;
    m_extensionToMimeTypeMap[FileExtension.EVY] = MimeType.APPLICATION_ENVOY;
    m_extensionToMimeTypeMap[FileExtension.EXE] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.F] = MimeType.TEXT_X_FORTRAN;
    m_extensionToMimeTypeMap[FileExtension.F77] = MimeType.TEXT_X_FORTRAN;
    m_extensionToMimeTypeMap[FileExtension.F90] = MimeType.TEXT_X_FORTRAN;
    m_extensionToMimeTypeMap[FileExtension.FDF] = MimeType.APPLICATION_VND_FDF;
    m_extensionToMimeTypeMap[FileExtension.FIF] = MimeType.IMAGE_FIF;
    m_extensionToMimeTypeMap[FileExtension.FLI] = MimeType.VIDEO_FLI;
    m_extensionToMimeTypeMap[FileExtension.FLO] = MimeType.IMAGE_FLORIAN;
    m_extensionToMimeTypeMap[FileExtension.FLX] = MimeType.TEXT_VND_FMI_FLEXSTOR;
    m_extensionToMimeTypeMap[FileExtension.FMF] = MimeType.VIDEO_X_ATOMIC3D_FEATURE;
    m_extensionToMimeTypeMap[FileExtension.FOR] = MimeType.TEXT_X_FORTRAN;
    m_extensionToMimeTypeMap[FileExtension.FPX] = MimeType.IMAGE_VND_FPX;
    m_extensionToMimeTypeMap[FileExtension.FRL] = MimeType.APPLICATION_FREELOADER;
    m_extensionToMimeTypeMap[FileExtension.FUNK] = MimeType.AUDIO_MAKE;
    m_extensionToMimeTypeMap[FileExtension.G] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.G3] = MimeType.IMAGE_G3FAX;
    m_extensionToMimeTypeMap[FileExtension.GIF] = MimeType.IMAGE_GIF;
    m_extensionToMimeTypeMap[FileExtension.GL] = MimeType.VIDEO_GL;
    m_extensionToMimeTypeMap[FileExtension.GLB] = MimeType.MODEL_GLTF_BINARY;
    m_extensionToMimeTypeMap[FileExtension.GLTF] = MimeType.MODEL_GLTF_JSON;
    m_extensionToMimeTypeMap[FileExtension.GSD] = MimeType.AUDIO_X_GSM;
    m_extensionToMimeTypeMap[FileExtension.GSM] = MimeType.AUDIO_X_GSM;
    m_extensionToMimeTypeMap[FileExtension.GSP] = MimeType.APPLICATION_X_GSP;
    m_extensionToMimeTypeMap[FileExtension.GSS] = MimeType.APPLICATION_X_GSS;
    m_extensionToMimeTypeMap[FileExtension.GTAR] = MimeType.APPLICATION_X_GTAR;
    m_extensionToMimeTypeMap[FileExtension.GZ] = MimeType.APPLICATION_X_COMPRESSED;
    m_extensionToMimeTypeMap[FileExtension.GZIP] = MimeType.MULTIPART_X_GZIP;
    m_extensionToMimeTypeMap[FileExtension.H] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.HDF] = MimeType.APPLICATION_X_HDF;
    m_extensionToMimeTypeMap[FileExtension.HELP] = MimeType.APPLICATION_X_HELPFILE;
    m_extensionToMimeTypeMap[FileExtension.HGL] = MimeType.APPLICATION_VND_HP_HPGL;
    m_extensionToMimeTypeMap[FileExtension.HH] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.HLB] = MimeType.TEXT_X_SCRIPT;
    m_extensionToMimeTypeMap[FileExtension.HLP] = MimeType.APPLICATION_HLP;
    m_extensionToMimeTypeMap[FileExtension.HPG] = MimeType.APPLICATION_VND_HP_HPGL;
    m_extensionToMimeTypeMap[FileExtension.HPGL] = MimeType.APPLICATION_VND_HP_HPGL;
    m_extensionToMimeTypeMap[FileExtension.HQX] = MimeType.APPLICATION_BINHEX;
    m_extensionToMimeTypeMap[FileExtension.HTA] = MimeType.APPLICATION_HTA;
    m_extensionToMimeTypeMap[FileExtension.HTC] = MimeType.TEXT_X_COMPONENT;
    m_extensionToMimeTypeMap[FileExtension.HTM] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.HTML] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.HTMLS] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.HTT] = MimeType.TEXT_WEBVIEWHTML;
    m_extensionToMimeTypeMap[FileExtension.HTX] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.ICE] = MimeType.X_CONFERENCE_X_COOLTALK;
    m_extensionToMimeTypeMap[FileExtension.ICO] = MimeType.IMAGE_X_ICON;
    m_extensionToMimeTypeMap[FileExtension.IDC] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.IEF] = MimeType.IMAGE_IEF;
    m_extensionToMimeTypeMap[FileExtension.IEFS] = MimeType.IMAGE_IEF;
    m_extensionToMimeTypeMap[FileExtension.IGES] = MimeType.MODEL_IGES;
    m_extensionToMimeTypeMap[FileExtension.IGS] = MimeType.MODEL_IGES;
    m_extensionToMimeTypeMap[FileExtension.IMA] = MimeType.APPLICATION_X_IMA;
    m_extensionToMimeTypeMap[FileExtension.IMAP] = MimeType.APPLICATION_X_HTTPD_IMAP;
    m_extensionToMimeTypeMap[FileExtension.INF] = MimeType.APPLICATION_INF;
    m_extensionToMimeTypeMap[FileExtension.INS] = MimeType.APPLICATION_X_INTERNETT_SIGNUP;
    m_extensionToMimeTypeMap[FileExtension.IP] = MimeType.APPLICATION_X_IP2;
    m_extensionToMimeTypeMap[FileExtension.ISU] = MimeType.VIDEO_X_ISVIDEO;
    m_extensionToMimeTypeMap[FileExtension.IT] = MimeType.AUDIO_IT;
    m_extensionToMimeTypeMap[FileExtension.IV] = MimeType.APPLICATION_X_INVENTOR;
    m_extensionToMimeTypeMap[FileExtension.IVR] = MimeType.I_WORLD_I_VRML;
    m_extensionToMimeTypeMap[FileExtension.IVY] = MimeType.APPLICATION_X_LIVESCREEN;
    m_extensionToMimeTypeMap[FileExtension.JAM] = MimeType.AUDIO_X_JAM;
    m_extensionToMimeTypeMap[FileExtension.JAV] = MimeType.TEXT_X_JAVA_SOURCE;
    m_extensionToMimeTypeMap[FileExtension.JAVA] = MimeType.TEXT_X_JAVA_SOURCE;
    m_extensionToMimeTypeMap[FileExtension.JCM] = MimeType.APPLICATION_X_JAVA_COMMERCE;
    m_extensionToMimeTypeMap[FileExtension.JFIF] = MimeType.IMAGE_JPEG;
    m_extensionToMimeTypeMap[FileExtension.JPE] = MimeType.IMAGE_JPEG;
    m_extensionToMimeTypeMap[FileExtension.JPEG] = MimeType.IMAGE_JPEG;
    m_extensionToMimeTypeMap[FileExtension.JPG] = MimeType.IMAGE_JPEG;
    m_extensionToMimeTypeMap[FileExtension.JPS] = MimeType.IMAGE_X_JPS;
    m_extensionToMimeTypeMap[FileExtension.JS] = MimeType.APPLICATION_JAVASCRIPT;
    m_extensionToMimeTypeMap[FileExtension.JUT] = MimeType.IMAGE_JUTVISION;
    m_extensionToMimeTypeMap[FileExtension.KAR] = MimeType.AUDIO_MIDI;
    m_extensionToMimeTypeMap[FileExtension.KSH] = MimeType.APPLICATION_X_KSH;
    m_extensionToMimeTypeMap[FileExtension.LA] = MimeType.AUDIO_NSPAUDIO;
    m_extensionToMimeTypeMap[FileExtension.LAM] = MimeType.AUDIO_X_LIVEAUDIO;
    m_extensionToMimeTypeMap[FileExtension.LATEX] = MimeType.APPLICATION_X_LATEX;
    m_extensionToMimeTypeMap[FileExtension.LHA] = MimeType.APPLICATION_LHA;
    m_extensionToMimeTypeMap[FileExtension.LHX] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.LIST] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.LMA] = MimeType.AUDIO_NSPAUDIO;
    m_extensionToMimeTypeMap[FileExtension.LOG] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.LSP] = MimeType.APPLICATION_X_LISP;
    m_extensionToMimeTypeMap[FileExtension.LST] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.LSX] = MimeType.TEXT_X_LA_ASF;
    m_extensionToMimeTypeMap[FileExtension.LTX] = MimeType.APPLICATION_X_LATEX;
    m_extensionToMimeTypeMap[FileExtension.LZH] = MimeType.APPLICATION_X_LZH;
    m_extensionToMimeTypeMap[FileExtension.LZX] = MimeType.APPLICATION_LZX;
    m_extensionToMimeTypeMap[FileExtension.M] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.M1V] = MimeType.VIDEO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.M2A] = MimeType.AUDIO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.M2V] = MimeType.VIDEO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.M3U] = MimeType.AUDIO_X_MPEQURL;
    m_extensionToMimeTypeMap[FileExtension.MAN] = MimeType.APPLICATION_X_TROFF_MAN;
    m_extensionToMimeTypeMap[FileExtension.MAP] = MimeType.APPLICATION_X_NAVIMAP;
    m_extensionToMimeTypeMap[FileExtension.MAR] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.MBD] = MimeType.APPLICATION_MBEDLET;
    m_extensionToMimeTypeMap[FileExtension.MC_DOLLAR] = MimeType.APPLICATION_X_MAGIC_CAP_PACKAGE_1_0;
    m_extensionToMimeTypeMap[FileExtension.MCD] = MimeType.APPLICATION_X_MATHCAD;
    m_extensionToMimeTypeMap[FileExtension.MCF] = MimeType.TEXT_MCF;
    m_extensionToMimeTypeMap[FileExtension.MCP] = MimeType.APPLICATION_NETMC;
    m_extensionToMimeTypeMap[FileExtension.ME] = MimeType.APPLICATION_X_TROFF_ME;
    m_extensionToMimeTypeMap[FileExtension.MHT] = MimeType.MESSAGE_RFC822;
    m_extensionToMimeTypeMap[FileExtension.MHTML] = MimeType.MESSAGE_RFC822;
    m_extensionToMimeTypeMap[FileExtension.MID] = MimeType.AUDIO_MIDI;
    m_extensionToMimeTypeMap[FileExtension.MIDI] = MimeType.AUDIO_MIDI;
    m_extensionToMimeTypeMap[FileExtension.MIF] = MimeType.APPLICATION_X_MIF;
    m_extensionToMimeTypeMap[FileExtension.MIME] = MimeType.WWW_MIME;
    m_extensionToMimeTypeMap[FileExtension.MJF] = MimeType.AUDIO_X_VND_AUDIOEXPLOSION_MJUICEMEDIAFILE;
    m_extensionToMimeTypeMap[FileExtension.MJPG] = MimeType.VIDEO_X_MOTION_JPEG;
    m_extensionToMimeTypeMap[FileExtension.MM] = MimeType.APPLICATION_BASE64;
    m_extensionToMimeTypeMap[FileExtension.MME] = MimeType.APPLICATION_BASE64;
    m_extensionToMimeTypeMap[FileExtension.MOD] = MimeType.AUDIO_MOD;
    m_extensionToMimeTypeMap[FileExtension.MOOV] = MimeType.VIDEO_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.MOV] = MimeType.VIDEO_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.MOVIE] = MimeType.VIDEO_X_SGI_MOVIE;
    m_extensionToMimeTypeMap[FileExtension.MP2] = MimeType.AUDIO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MP3] = MimeType.AUDIO_MPEG3;
    m_extensionToMimeTypeMap[FileExtension.MP4] = MimeType.VIDEO_MP4;
    m_extensionToMimeTypeMap[FileExtension.MPA] = MimeType.AUDIO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MPC] = MimeType.APPLICATION_X_PROJECT;
    m_extensionToMimeTypeMap[FileExtension.MPE] = MimeType.VIDEO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MPEG] = MimeType.VIDEO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MPG] = MimeType.VIDEO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MPGA] = MimeType.AUDIO_MPEG;
    m_extensionToMimeTypeMap[FileExtension.MPP] = MimeType.APPLICATION_VND_MS_PROJECT;
    m_extensionToMimeTypeMap[FileExtension.MPT] = MimeType.APPLICATION_X_PROJECT;
    m_extensionToMimeTypeMap[FileExtension.MPV] = MimeType.APPLICATION_X_PROJECT;
    m_extensionToMimeTypeMap[FileExtension.MPX] = MimeType.APPLICATION_X_PROJECT;
    m_extensionToMimeTypeMap[FileExtension.MRC] = MimeType.APPLICATION_MARC;
    m_extensionToMimeTypeMap[FileExtension.MS] = MimeType.APPLICATION_X_TROFF_MS;
    m_extensionToMimeTypeMap[FileExtension.MV] = MimeType.VIDEO_X_SGI_MOVIE;
    m_extensionToMimeTypeMap[FileExtension.MY] = MimeType.AUDIO_MAKE;
    m_extensionToMimeTypeMap[FileExtension.MZZ] = MimeType.APPLICATION_X_VND_AUDIOEXPLOSION_MZZ;
    m_extensionToMimeTypeMap[FileExtension.NAP] = MimeType.IMAGE_NAPLPS;
    m_extensionToMimeTypeMap[FileExtension.NAPLPS] = MimeType.IMAGE_NAPLPS;
    m_extensionToMimeTypeMap[FileExtension.NC] = MimeType.APPLICATION_X_NETCDF;
    m_extensionToMimeTypeMap[FileExtension.NCM] = MimeType.APPLICATION_VND_NOKIA_CONFIGURATION_MESSAGE;
    m_extensionToMimeTypeMap[FileExtension.NIF] = MimeType.IMAGE_X_NIFF;
    m_extensionToMimeTypeMap[FileExtension.NIFF] = MimeType.IMAGE_X_NIFF;
    m_extensionToMimeTypeMap[FileExtension.NIX] = MimeType.APPLICATION_X_MIX_TRANSFER;
    m_extensionToMimeTypeMap[FileExtension.NSC] = MimeType.APPLICATION_X_CONFERENCE;
    m_extensionToMimeTypeMap[FileExtension.NVD] = MimeType.APPLICATION_X_NAVIDOC;
    m_extensionToMimeTypeMap[FileExtension.O] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.ODA] = MimeType.APPLICATION_ODA;
    m_extensionToMimeTypeMap[FileExtension.OMC] = MimeType.APPLICATION_X_OMC;
    m_extensionToMimeTypeMap[FileExtension.OMCD] = MimeType.APPLICATION_X_OMCDATAMAKER;
    m_extensionToMimeTypeMap[FileExtension.OMCR] = MimeType.APPLICATION_X_OMCREGERATOR;
    m_extensionToMimeTypeMap[FileExtension.P] = MimeType.TEXT_X_PASCAL;
    m_extensionToMimeTypeMap[FileExtension.P10] = MimeType.APPLICATION_PKCS10;
    m_extensionToMimeTypeMap[FileExtension.P12] = MimeType.APPLICATION_PKCS_12;
    m_extensionToMimeTypeMap[FileExtension.P7A] = MimeType.APPLICATION_X_PKCS7_SIGNATURE;
    m_extensionToMimeTypeMap[FileExtension.P7C] = MimeType.APPLICATION_PKCS7_MIME;
    m_extensionToMimeTypeMap[FileExtension.P7M] = MimeType.APPLICATION_PKCS7_MIME;
    m_extensionToMimeTypeMap[FileExtension.P7R] = MimeType.APPLICATION_X_PKCS7_CERTREQRESP;
    m_extensionToMimeTypeMap[FileExtension.P7S] = MimeType.APPLICATION_PKCS7_SIGNATURE;
    m_extensionToMimeTypeMap[FileExtension.PART] = MimeType.APPLICATION_PRO_ENG;
    m_extensionToMimeTypeMap[FileExtension.PAS] = MimeType.TEXT_PASCAL;
    m_extensionToMimeTypeMap[FileExtension.PBM] = MimeType.IMAGE_X_PORDIV_BITMAP;
    m_extensionToMimeTypeMap[FileExtension.PCL] = MimeType.APPLICATION_X_PCL;
    m_extensionToMimeTypeMap[FileExtension.PCT] = MimeType.IMAGE_X_PICT;
    m_extensionToMimeTypeMap[FileExtension.PCX] = MimeType.IMAGE_X_PCX;
    m_extensionToMimeTypeMap[FileExtension.PDB] = MimeType.CHEMICAL_X_PDB;
    m_extensionToMimeTypeMap[FileExtension.PDF] = MimeType.APPLICATION_PDF;
    m_extensionToMimeTypeMap[FileExtension.PFUNK] = MimeType.AUDIO_MAKE;
    m_extensionToMimeTypeMap[FileExtension.PGM] = MimeType.IMAGE_X_PORDIV_GRAYMAP;
    m_extensionToMimeTypeMap[FileExtension.PIC] = MimeType.IMAGE_PICT;
    m_extensionToMimeTypeMap[FileExtension.PICT] = MimeType.IMAGE_PICT;
    m_extensionToMimeTypeMap[FileExtension.PKG] = MimeType.APPLICATION_X_NEWTON_COMPATIBLE_PKG;
    m_extensionToMimeTypeMap[FileExtension.PKO] = MimeType.APPLICATION_VND_MS_PKI_PKO;
    m_extensionToMimeTypeMap[FileExtension.PL] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.PLX] = MimeType.APPLICATION_X_PIXCLSCRIPT;
    m_extensionToMimeTypeMap[FileExtension.PM] = MimeType.IMAGE_X_XPIXMAP;
    m_extensionToMimeTypeMap[FileExtension.PM4] = MimeType.APPLICATION_X_PAGEMAKER;
    m_extensionToMimeTypeMap[FileExtension.PM5] = MimeType.APPLICATION_X_PAGEMAKER;
    m_extensionToMimeTypeMap[FileExtension.PNG] = MimeType.IMAGE_PNG;
    m_extensionToMimeTypeMap[FileExtension.PNM] = MimeType.IMAGE_X_PORDIV_ANYMAP;
    m_extensionToMimeTypeMap[FileExtension.POT] = MimeType.APPLICATION_MSPOWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.POV] = MimeType.MODEL_X_POV;
    m_extensionToMimeTypeMap[FileExtension.PPA] = MimeType.APPLICATION_VND_MS_POWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.PPM] = MimeType.IMAGE_X_PORDIV_PIXMAP;
    m_extensionToMimeTypeMap[FileExtension.PPS] = MimeType.APPLICATION_MSPOWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.PPT] = MimeType.APPLICATION_POWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.PPZ] = MimeType.APPLICATION_MSPOWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.PRE] = MimeType.APPLICATION_X_FREELANCE;
    m_extensionToMimeTypeMap[FileExtension.PRT] = MimeType.APPLICATION_PRO_ENG;
    m_extensionToMimeTypeMap[FileExtension.PS] = MimeType.APPLICATION_POSTSCRIPT;
    m_extensionToMimeTypeMap[FileExtension.PSD] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.PVU] = MimeType.PALEOVU_X_PV;
    m_extensionToMimeTypeMap[FileExtension.PWZ] = MimeType.APPLICATION_VND_MS_POWERPOINT;
    m_extensionToMimeTypeMap[FileExtension.PY] = MimeType.TEXT_X_SCRIPT_PHYTON;
    m_extensionToMimeTypeMap[FileExtension.PYC] = MimeType.APPLICATION_X_BYTECODE_PYTHON;
    m_extensionToMimeTypeMap[FileExtension.QCP] = MimeType.AUDIO_VND_QCELP;
    m_extensionToMimeTypeMap[FileExtension.QD3] = MimeType.X_WORLD_X_3DMF;
    m_extensionToMimeTypeMap[FileExtension.QD3D] = MimeType.X_WORLD_X_3DMF;
    m_extensionToMimeTypeMap[FileExtension.QIF] = MimeType.IMAGE_X_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.QT] = MimeType.VIDEO_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.QTC] = MimeType.VIDEO_X_QTC;
    m_extensionToMimeTypeMap[FileExtension.QTI] = MimeType.IMAGE_X_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.QTIF] = MimeType.IMAGE_X_QUICKTIME;
    m_extensionToMimeTypeMap[FileExtension.RA] = MimeType.AUDIO_X_REALAUDIO;
    m_extensionToMimeTypeMap[FileExtension.RAM] = MimeType.AUDIO_X_PN_REALAUDIO;
    m_extensionToMimeTypeMap[FileExtension.RAS] = MimeType.IMAGE_CMU_RASTER;
    m_extensionToMimeTypeMap[FileExtension.RAST] = MimeType.IMAGE_CMU_RASTER;
    m_extensionToMimeTypeMap[FileExtension.REXX] = MimeType.TEXT_X_SCRIPT_REXX;
    m_extensionToMimeTypeMap[FileExtension.RF] = MimeType.IMAGE_VND_RN_REALFLASH;
    m_extensionToMimeTypeMap[FileExtension.RGB] = MimeType.IMAGE_X_RGB;
    m_extensionToMimeTypeMap[FileExtension.RM] = MimeType.AUDIO_X_PN_REALAUDIO;
    m_extensionToMimeTypeMap[FileExtension.RMI] = MimeType.AUDIO_MID;
    m_extensionToMimeTypeMap[FileExtension.RMM] = MimeType.AUDIO_X_PN_REALAUDIO;
    m_extensionToMimeTypeMap[FileExtension.RMP] = MimeType.AUDIO_X_PN_REALAUDIO;
    m_extensionToMimeTypeMap[FileExtension.RNG] = MimeType.APPLICATION_RINGING_TONES;
    m_extensionToMimeTypeMap[FileExtension.RNX] = MimeType.APPLICATION_VND_RN_REALPLAYER;
    m_extensionToMimeTypeMap[FileExtension.ROFF] = MimeType.APPLICATION_X_TROFF;
    m_extensionToMimeTypeMap[FileExtension.RP] = MimeType.IMAGE_VND_RN_REALPIX;
    m_extensionToMimeTypeMap[FileExtension.RPM] = MimeType.AUDIO_X_PN_REALAUDIO_PLUGIN;
    m_extensionToMimeTypeMap[FileExtension.RT] = MimeType.TEXT_RICHTEXT;
    m_extensionToMimeTypeMap[FileExtension.RTF] = MimeType.APPLICATION_RTF;
    m_extensionToMimeTypeMap[FileExtension.RTX] = MimeType.APPLICATION_RTF;
    m_extensionToMimeTypeMap[FileExtension.RV] = MimeType.VIDEO_VND_RN_REALVIDEO;
    m_extensionToMimeTypeMap[FileExtension.S] = MimeType.TEXT_X_ASM;
    m_extensionToMimeTypeMap[FileExtension.S3M] = MimeType.AUDIO_S3M;
    m_extensionToMimeTypeMap[FileExtension.SAVEME] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.SBK] = MimeType.APPLICATION_X_TBOOK;
    m_extensionToMimeTypeMap[FileExtension.SCM] = MimeType.VIDEO_X_SCM;
    m_extensionToMimeTypeMap[FileExtension.SDML] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.SDP] = MimeType.APPLICATION_SDP;
    m_extensionToMimeTypeMap[FileExtension.SDR] = MimeType.APPLICATION_SOUNDER;
    m_extensionToMimeTypeMap[FileExtension.SEA] = MimeType.APPLICATION_SEA;
    m_extensionToMimeTypeMap[FileExtension.SET] = MimeType.APPLICATION_SET;
    m_extensionToMimeTypeMap[FileExtension.SGM] = MimeType.TEXT_SGML;
    m_extensionToMimeTypeMap[FileExtension.SGML] = MimeType.TEXT_SGML;
    m_extensionToMimeTypeMap[FileExtension.SH] = MimeType.APPLICATION_X_SH;
    m_extensionToMimeTypeMap[FileExtension.SHAR] = MimeType.APPLICATION_X_SHAR;
    m_extensionToMimeTypeMap[FileExtension.SHTML] = MimeType.TEXT_HTML;
    m_extensionToMimeTypeMap[FileExtension.SID] = MimeType.AUDIO_X_PSID;
    m_extensionToMimeTypeMap[FileExtension.SIT] = MimeType.APPLICATION_X_SIT;
    m_extensionToMimeTypeMap[FileExtension.SKD] = MimeType.APPLICATION_X_KOAN;
    m_extensionToMimeTypeMap[FileExtension.SKM] = MimeType.APPLICATION_X_KOAN;
    m_extensionToMimeTypeMap[FileExtension.SKP] = MimeType.APPLICATION_X_KOAN;
    m_extensionToMimeTypeMap[FileExtension.SKT] = MimeType.APPLICATION_X_KOAN;
    m_extensionToMimeTypeMap[FileExtension.SL] = MimeType.APPLICATION_X_SEELOGO;
    m_extensionToMimeTypeMap[FileExtension.SMI] = MimeType.APPLICATION_SMIL;
    m_extensionToMimeTypeMap[FileExtension.SMIL] = MimeType.APPLICATION_SMIL;
    m_extensionToMimeTypeMap[FileExtension.SND] = MimeType.AUDIO_BASIC;
    m_extensionToMimeTypeMap[FileExtension.SOL] = MimeType.APPLICATION_SOLIDS;
    m_extensionToMimeTypeMap[FileExtension.SPC] = MimeType.TEXT_X_SPEECH;
    m_extensionToMimeTypeMap[FileExtension.SPL] = MimeType.APPLICATION_FUTURESPLASH;
    m_extensionToMimeTypeMap[FileExtension.SPR] = MimeType.APPLICATION_X_SPRITE;
    m_extensionToMimeTypeMap[FileExtension.SPRITE] = MimeType.APPLICATION_X_SPRITE;
    m_extensionToMimeTypeMap[FileExtension.SRC] = MimeType.APPLICATION_X_WAIS_SOURCE;
    m_extensionToMimeTypeMap[FileExtension.SSI] = MimeType.TEXT_X_SERVER_PARSED_HTML;
    m_extensionToMimeTypeMap[FileExtension.SSM] = MimeType.APPLICATION_STREAMINGMEDIA;
    m_extensionToMimeTypeMap[FileExtension.SST] = MimeType.APPLICATION_VND_MS_PKI_CERTSTORE;
    m_extensionToMimeTypeMap[FileExtension.STEP] = MimeType.APPLICATION_STEP;
    m_extensionToMimeTypeMap[FileExtension.STL] = MimeType.APPLICATION_SLA;
    m_extensionToMimeTypeMap[FileExtension.STP] = MimeType.APPLICATION_STEP;
    m_extensionToMimeTypeMap[FileExtension.SV4CPIO] = MimeType.APPLICATION_X_SV4CPIO;
    m_extensionToMimeTypeMap[FileExtension.SV4CRC] = MimeType.APPLICATION_X_SV4CRC;
    m_extensionToMimeTypeMap[FileExtension.SVF] = MimeType.IMAGE_VND_DWG;
    m_extensionToMimeTypeMap[FileExtension.SVR] = MimeType.APPLICATION_X_WORLD;
    m_extensionToMimeTypeMap[FileExtension.SWF] = MimeType.APPLICATION_X_SHOCKWAVE_FLASH;
    m_extensionToMimeTypeMap[FileExtension.T] = MimeType.APPLICATION_X_TROFF;
    m_extensionToMimeTypeMap[FileExtension.TALK] = MimeType.TEXT_X_SPEECH;
    m_extensionToMimeTypeMap[FileExtension.TAR] = MimeType.APPLICATION_X_TAR;
    m_extensionToMimeTypeMap[FileExtension.TBK] = MimeType.APPLICATION_TOOLBOOK;
    m_extensionToMimeTypeMap[FileExtension.TCL] = MimeType.APPLICATION_X_TCL;
    m_extensionToMimeTypeMap[FileExtension.TCSH] = MimeType.TEXT_X_SCRIPT_TCSH;
    m_extensionToMimeTypeMap[FileExtension.TEX] = MimeType.APPLICATION_X_TEX;
    m_extensionToMimeTypeMap[FileExtension.TEXI] = MimeType.APPLICATION_X_TEXINFO;
    m_extensionToMimeTypeMap[FileExtension.TEXINFO] = MimeType.APPLICATION_X_TEXINFO;
    m_extensionToMimeTypeMap[FileExtension.TEXT] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.TGZ] = MimeType.APPLICATION_GNUTAR;
    m_extensionToMimeTypeMap[FileExtension.TIF] = MimeType.IMAGE_TIFF;
    m_extensionToMimeTypeMap[FileExtension.TIFF] = MimeType.IMAGE_TIFF;
    m_extensionToMimeTypeMap[FileExtension.TR] = MimeType.APPLICATION_X_TROFF;
    m_extensionToMimeTypeMap[FileExtension.TSI] = MimeType.AUDIO_TSP_AUDIO;
    m_extensionToMimeTypeMap[FileExtension.TSP] = MimeType.APPLICATION_DSPTYPE;
    m_extensionToMimeTypeMap[FileExtension.TSV] = MimeType.TEXT_TAB_SEPARATED_VALUES;
    m_extensionToMimeTypeMap[FileExtension.TURBOT] = MimeType.IMAGE_FLORIAN;
    m_extensionToMimeTypeMap[FileExtension.TXT] = MimeType.TEXT_PLAIN;
    m_extensionToMimeTypeMap[FileExtension.UIL] = MimeType.TEXT_X_UIL;
    m_extensionToMimeTypeMap[FileExtension.UNI] = MimeType.TEXT_URI_LIST;
    m_extensionToMimeTypeMap[FileExtension.UNIS] = MimeType.TEXT_URI_LIST;
    m_extensionToMimeTypeMap[FileExtension.UNV] = MimeType.APPLICATION_I_DEAS;
    m_extensionToMimeTypeMap[FileExtension.URI] = MimeType.TEXT_URI_LIST;
    m_extensionToMimeTypeMap[FileExtension.URIS] = MimeType.TEXT_URI_LIST;
    m_extensionToMimeTypeMap[FileExtension.USDZ] = MimeType.MODEL_VND_USDZ_ZIP;
    m_extensionToMimeTypeMap[FileExtension.USTAR] = MimeType.APPLICATION_X_USTAR;
    m_extensionToMimeTypeMap[FileExtension.UU] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.UUE] = MimeType.TEXT_X_UUENCODE;
    m_extensionToMimeTypeMap[FileExtension.VCD] = MimeType.APPLICATION_X_CDLINK;
    m_extensionToMimeTypeMap[FileExtension.VCS] = MimeType.TEXT_X_VCALENDAR;
    m_extensionToMimeTypeMap[FileExtension.VDA] = MimeType.APPLICATION_VDA;
    m_extensionToMimeTypeMap[FileExtension.VDO] = MimeType.VIDEO_VDO;
    m_extensionToMimeTypeMap[FileExtension.VEW] = MimeType.APPLICATION_GROUPWISE;
    m_extensionToMimeTypeMap[FileExtension.VIV] = MimeType.VIDEO_VIVO;
    m_extensionToMimeTypeMap[FileExtension.VIVO] = MimeType.VIDEO_VIVO;
    m_extensionToMimeTypeMap[FileExtension.VMD] = MimeType.APPLICATION_VOCALTEC_MEDIA_DESC;
    m_extensionToMimeTypeMap[FileExtension.VMF] = MimeType.APPLICATION_VOCALTEC_MEDIA_FILE;
    m_extensionToMimeTypeMap[FileExtension.VOC] = MimeType.AUDIO_VOC;
    m_extensionToMimeTypeMap[FileExtension.VOS] = MimeType.VIDEO_VOSAIC;
    m_extensionToMimeTypeMap[FileExtension.VOX] = MimeType.AUDIO_VOXWARE;
    m_extensionToMimeTypeMap[FileExtension.VQE] = MimeType.AUDIO_X_TWINVQ_PLUGIN;
    m_extensionToMimeTypeMap[FileExtension.VQF] = MimeType.AUDIO_X_TWINVQ;
    m_extensionToMimeTypeMap[FileExtension.VQL] = MimeType.AUDIO_X_TWINVQ_PLUGIN;
    m_extensionToMimeTypeMap[FileExtension.VRML] = MimeType.MODEL_VRML;
    m_extensionToMimeTypeMap[FileExtension.VRT] = MimeType.X_WORLD_X_VRT;
    m_extensionToMimeTypeMap[FileExtension.VSD] = MimeType.APPLICATION_X_VISIO;
    m_extensionToMimeTypeMap[FileExtension.VST] = MimeType.APPLICATION_X_VISIO;
    m_extensionToMimeTypeMap[FileExtension.VSW] = MimeType.APPLICATION_X_VISIO;
    m_extensionToMimeTypeMap[FileExtension.W60] = MimeType.APPLICATION_WORDPERFECT6_0;
    m_extensionToMimeTypeMap[FileExtension.W61] = MimeType.APPLICATION_WORDPERFECT6_1;
    m_extensionToMimeTypeMap[FileExtension.W6W] = MimeType.APPLICATION_MSWORD;
    m_extensionToMimeTypeMap[FileExtension.WAV] = MimeType.AUDIO_WAV;
    m_extensionToMimeTypeMap[FileExtension.WB1] = MimeType.APPLICATION_X_QPRO;
    m_extensionToMimeTypeMap[FileExtension.WBMP] = MimeType.IMAGE_VND_WAP_WBMP;
    m_extensionToMimeTypeMap[FileExtension.WEB] = MimeType.APPLICATION_VND_XARA;
    m_extensionToMimeTypeMap[FileExtension.WIZ] = MimeType.APPLICATION_MSWORD;
    m_extensionToMimeTypeMap[FileExtension.WK1] = MimeType.APPLICATION_X_123;
    m_extensionToMimeTypeMap[FileExtension.WMF] = MimeType.WINDOWS_METAFILE;
    m_extensionToMimeTypeMap[FileExtension.WML] = MimeType.TEXT_VND_WAP_WML;
    m_extensionToMimeTypeMap[FileExtension.WMLC] = MimeType.APPLICATION_VND_WAP_WMLC;
    m_extensionToMimeTypeMap[FileExtension.WMLS] = MimeType.TEXT_VND_WAP_WMLSCRIPT;
    m_extensionToMimeTypeMap[FileExtension.WMLSC] = MimeType.APPLICATION_VND_WAP_WMLSCRIPTC;
    m_extensionToMimeTypeMap[FileExtension.WORD] = MimeType.APPLICATION_MSWORD;
    m_extensionToMimeTypeMap[FileExtension.WP] = MimeType.APPLICATION_WORDPERFECT;
    m_extensionToMimeTypeMap[FileExtension.WP5] = MimeType.APPLICATION_WORDPERFECT;
    m_extensionToMimeTypeMap[FileExtension.WP6] = MimeType.APPLICATION_WORDPERFECT;
    m_extensionToMimeTypeMap[FileExtension.WPD] = MimeType.APPLICATION_WORDPERFECT;
    m_extensionToMimeTypeMap[FileExtension.WQ1] = MimeType.APPLICATION_X_LOTUS;
    m_extensionToMimeTypeMap[FileExtension.WRI] = MimeType.APPLICATION_MSWRITE;
    m_extensionToMimeTypeMap[FileExtension.WRL] = MimeType.APPLICATION_X_WORLD;
    m_extensionToMimeTypeMap[FileExtension.WRZ] = MimeType.MODEL_VRML;
    m_extensionToMimeTypeMap[FileExtension.WSC] = MimeType.TEXT_SCRIPLET;
    m_extensionToMimeTypeMap[FileExtension.WSRC] = MimeType.APPLICATION_X_WAIS_SOURCE;
    m_extensionToMimeTypeMap[FileExtension.WTK] = MimeType.APPLICATION_X_WINTALK;
    m_extensionToMimeTypeMap[FileExtension.XBM] = MimeType.IMAGE_X_XBITMAP;
    m_extensionToMimeTypeMap[FileExtension.XDR] = MimeType.VIDEO_X_AMT_DEMORUN;
    m_extensionToMimeTypeMap[FileExtension.XGZ] = MimeType.XGL_DRAWING;
    m_extensionToMimeTypeMap[FileExtension.XIF] = MimeType.IMAGE_VND_XIFF;
    m_extensionToMimeTypeMap[FileExtension.XL] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLA] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLB] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLC] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLD] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLK] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLL] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLM] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLS] = MimeType.APPLICATION_X_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLT] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLV] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XLW] = MimeType.APPLICATION_EXCEL;
    m_extensionToMimeTypeMap[FileExtension.XM] = MimeType.AUDIO_XM;
    m_extensionToMimeTypeMap[FileExtension.XML] = MimeType.APPLICATION_XML;
    m_extensionToMimeTypeMap[FileExtension.XPIX] = MimeType.APPLICATION_X_VND_LS_XPIX;
    m_extensionToMimeTypeMap[FileExtension.XPM] = MimeType.IMAGE_X_XPIXMAP;
    m_extensionToMimeTypeMap[FileExtension.X_PNG] = MimeType.IMAGE_PNG;
    m_extensionToMimeTypeMap[FileExtension.XSR] = MimeType.VIDEO_X_AMT_SHOWRUN;
    m_extensionToMimeTypeMap[FileExtension.XWD] = MimeType.IMAGE_X_XWD;
    m_extensionToMimeTypeMap[FileExtension.XYZ] = MimeType.CHEMICAL_X_PDB;
    m_extensionToMimeTypeMap[FileExtension.Z] = MimeType.APPLICATION_X_COMPRESSED;
    m_extensionToMimeTypeMap[FileExtension.ZIP] = MimeType.APPLICATION_ZIP;
    m_extensionToMimeTypeMap[FileExtension.ZOO] = MimeType.APPLICATION_OCTET_STREAM;
    m_extensionToMimeTypeMap[FileExtension.ZSH] = MimeType.TEXT_X_SCRIPT_ZSH;
}

} // namespace csp::common
