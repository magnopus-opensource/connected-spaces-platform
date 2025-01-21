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

#include "Memory/Memory.h"

#include <cctype>

namespace csp::common
{

MimeTypeHelper& MimeTypeHelper::Get()
{
    static MimeTypeHelper Instance;
    return Instance;
}

String& MimeTypeHelper::GetMimeType(const String& FilePath)
{
    auto Segments = FilePath.Split('.');
    auto Extension = Segments[Segments.Size() - 1].Trim();
    auto Length = Extension.Length();
    auto Chars = Extension.c_str();

    auto LowerChars = std::make_unique<char[]>(Length);

    for (auto i = 0; i < Length; ++i)
    {
        LowerChars[i] = static_cast<char>(std::tolower(Chars[i]));
    }

    String LowerExtension(LowerChars.get(), Length);

    if (ExtensionToMimeTypeMap.HasKey(LowerExtension))
    {
        return ExtensionToMimeTypeMap[LowerExtension];
    }
    else
    {
        return Default;
    }
}

MimeTypes::MimeTypes() { }

FileExtensions::FileExtensions() { }

MimeTypeHelper::MimeTypeHelper()
{
    ExtensionToMimeTypeMap[FileExtension._3DM] = MimeType.X_WORLD_X_3DMF;
    ExtensionToMimeTypeMap[FileExtension._3DMF] = MimeType.X_WORLD_X_3DMF;
    ExtensionToMimeTypeMap[FileExtension.A] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.AAB] = MimeType.APPLICATION_X_AUTHORWARE_BIN;
    ExtensionToMimeTypeMap[FileExtension.AAM] = MimeType.APPLICATION_X_AUTHORWARE_MAPZ;
    ExtensionToMimeTypeMap[FileExtension.AAS] = MimeType.APPLICATION_X_AUTHORWARE_SEG;
    ExtensionToMimeTypeMap[FileExtension.ABC] = MimeType.TEXT_VND_ABC;
    ExtensionToMimeTypeMap[FileExtension.ACGI] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.AFL] = MimeType.VIDEO_ANIMAFLEX;
    ExtensionToMimeTypeMap[FileExtension.AI] = MimeType.APPLICATION_POSTSCRIPT;
    ExtensionToMimeTypeMap[FileExtension.AIF] = MimeType.AUDIO_AIFF;
    ExtensionToMimeTypeMap[FileExtension.AIFC] = MimeType.AUDIO_AIFF;
    ExtensionToMimeTypeMap[FileExtension.AIFF] = MimeType.AUDIO_AIFF;
    ExtensionToMimeTypeMap[FileExtension.AIM] = MimeType.APPLICATION_X_AIM;
    ExtensionToMimeTypeMap[FileExtension.AIP] = MimeType.TEXT_X_AUDIOSOFT_INTRA;
    ExtensionToMimeTypeMap[FileExtension.ANI] = MimeType.APPLICATION_X_NAVI_ANIMATION;
    ExtensionToMimeTypeMap[FileExtension.AOS] = MimeType.APPLICATION_X_NOKIA_9000_COMMUNICATOR_ADD_ON_SOFTWARE;
    ExtensionToMimeTypeMap[FileExtension.APS] = MimeType.APPLICATION_MIME;
    ExtensionToMimeTypeMap[FileExtension.ARC] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.ARJ] = MimeType.APPLICATION_ARJ;
    ExtensionToMimeTypeMap[FileExtension.ART] = MimeType.IMAGE_X_JG;
    ExtensionToMimeTypeMap[FileExtension.ASF] = MimeType.VIDEO_X_MS_ASF;
    ExtensionToMimeTypeMap[FileExtension.ASM] = MimeType.TEXT_X_ASM;
    ExtensionToMimeTypeMap[FileExtension.ASP] = MimeType.TEXT_ASP;
    ExtensionToMimeTypeMap[FileExtension.ASX] = MimeType.APPLICATION_X_MPLAYER2;
    ExtensionToMimeTypeMap[FileExtension.AU] = MimeType.AUDIO_BASIC;
    ExtensionToMimeTypeMap[FileExtension.AVI] = MimeType.VIDEO_AVI;
    ExtensionToMimeTypeMap[FileExtension.AVS] = MimeType.VIDEO_AVS_VIDEO;
    ExtensionToMimeTypeMap[FileExtension.BCPIO] = MimeType.APPLICATION_X_BCPIO;
    ExtensionToMimeTypeMap[FileExtension.BIN] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.BM] = MimeType.IMAGE_BMP;
    ExtensionToMimeTypeMap[FileExtension.BMP] = MimeType.IMAGE_BMP;
    ExtensionToMimeTypeMap[FileExtension.BOO] = MimeType.APPLICATION_BOOK;
    ExtensionToMimeTypeMap[FileExtension.BOOK] = MimeType.APPLICATION_BOOK;
    ExtensionToMimeTypeMap[FileExtension.BOZ] = MimeType.APPLICATION_X_BZIP2;
    ExtensionToMimeTypeMap[FileExtension.BSH] = MimeType.APPLICATION_X_BSH;
    ExtensionToMimeTypeMap[FileExtension.BZ] = MimeType.APPLICATION_X_BZIP;
    ExtensionToMimeTypeMap[FileExtension.BZ2] = MimeType.APPLICATION_X_BZIP2;
    ExtensionToMimeTypeMap[FileExtension.C] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.C_PLUS_PLUS] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.CAT] = MimeType.APPLICATION_VND_MS_PKI_SECCAT;
    ExtensionToMimeTypeMap[FileExtension.CC] = MimeType.TEXT_X_C;
    ExtensionToMimeTypeMap[FileExtension.CCAD] = MimeType.APPLICATION_CLARISCAD;
    ExtensionToMimeTypeMap[FileExtension.CCO] = MimeType.APPLICATION_X_COCOA;
    ExtensionToMimeTypeMap[FileExtension.CDF] = MimeType.APPLICATION_CDF;
    ExtensionToMimeTypeMap[FileExtension.CER] = MimeType.APPLICATION_PKIX_CERT;
    ExtensionToMimeTypeMap[FileExtension.CHA] = MimeType.APPLICATION_X_CHAT;
    ExtensionToMimeTypeMap[FileExtension.CHAT] = MimeType.APPLICATION_X_CHAT;
    ExtensionToMimeTypeMap[FileExtension.CLASS] = MimeType.APPLICATION_JAVA;
    ExtensionToMimeTypeMap[FileExtension.COM] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.CONF] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.CPIO] = MimeType.APPLICATION_X_CPIO;
    ExtensionToMimeTypeMap[FileExtension.CPP] = MimeType.TEXT_X_C;
    ExtensionToMimeTypeMap[FileExtension.CPT] = MimeType.APPLICATION_X_CPT;
    ExtensionToMimeTypeMap[FileExtension.CRL] = MimeType.APPLICATION_PKCS_CRL;
    ExtensionToMimeTypeMap[FileExtension.CRT] = MimeType.APPLICATION_PKIX_CERT;
    ExtensionToMimeTypeMap[FileExtension.CSH] = MimeType.APPLICATION_X_CSH;
    ExtensionToMimeTypeMap[FileExtension.CSS] = MimeType.APPLICATION_X_POINTPLUS;
    ExtensionToMimeTypeMap[FileExtension.CXX] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.DCR] = MimeType.APPLICATION_X_DIRECTOR;
    ExtensionToMimeTypeMap[FileExtension.DEEPV] = MimeType.APPLICATION_X_DEEPV;
    ExtensionToMimeTypeMap[FileExtension.DEF] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.DER] = MimeType.APPLICATION_X_X509_CA_CERT;
    ExtensionToMimeTypeMap[FileExtension.DIF] = MimeType.VIDEO_X_DV;
    ExtensionToMimeTypeMap[FileExtension.DIR] = MimeType.APPLICATION_X_DIRECTOR;
    ExtensionToMimeTypeMap[FileExtension.DL] = MimeType.VIDEO_DL;
    ExtensionToMimeTypeMap[FileExtension.DOC] = MimeType.APPLICATION_MSWORD;
    ExtensionToMimeTypeMap[FileExtension.DOT] = MimeType.APPLICATION_MSWORD;
    ExtensionToMimeTypeMap[FileExtension.DP] = MimeType.APPLICATION_COMMONGROUND;
    ExtensionToMimeTypeMap[FileExtension.DRW] = MimeType.APPLICATION_DRAFTING;
    ExtensionToMimeTypeMap[FileExtension.DUMP] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.DV] = MimeType.VIDEO_X_DV;
    ExtensionToMimeTypeMap[FileExtension.DVI] = MimeType.APPLICATION_X_DVI;
    ExtensionToMimeTypeMap[FileExtension.DWF] = MimeType.MODEL_VND_DWF;
    ExtensionToMimeTypeMap[FileExtension.DWG] = MimeType.APPLICATION_ACAD;
    ExtensionToMimeTypeMap[FileExtension.DXF] = MimeType.APPLICATION_DXF;
    ExtensionToMimeTypeMap[FileExtension.EL] = MimeType.TEXT_X_SCRIPT_ELISP;
    ExtensionToMimeTypeMap[FileExtension.ELC] = MimeType.APPLICATION_X_ELC;
    ExtensionToMimeTypeMap[FileExtension.ENV] = MimeType.APPLICATION_X_ENVOY;
    ExtensionToMimeTypeMap[FileExtension.EPS] = MimeType.APPLICATION_POSTSCRIPT;
    ExtensionToMimeTypeMap[FileExtension.ES] = MimeType.APPLICATION_X_ESREHBER;
    ExtensionToMimeTypeMap[FileExtension.ETX] = MimeType.TEXT_X_SETEXT;
    ExtensionToMimeTypeMap[FileExtension.EVY] = MimeType.APPLICATION_ENVOY;
    ExtensionToMimeTypeMap[FileExtension.EXE] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.F] = MimeType.TEXT_X_FORTRAN;
    ExtensionToMimeTypeMap[FileExtension.F77] = MimeType.TEXT_X_FORTRAN;
    ExtensionToMimeTypeMap[FileExtension.F90] = MimeType.TEXT_X_FORTRAN;
    ExtensionToMimeTypeMap[FileExtension.FDF] = MimeType.APPLICATION_VND_FDF;
    ExtensionToMimeTypeMap[FileExtension.FIF] = MimeType.IMAGE_FIF;
    ExtensionToMimeTypeMap[FileExtension.FLI] = MimeType.VIDEO_FLI;
    ExtensionToMimeTypeMap[FileExtension.FLO] = MimeType.IMAGE_FLORIAN;
    ExtensionToMimeTypeMap[FileExtension.FLX] = MimeType.TEXT_VND_FMI_FLEXSTOR;
    ExtensionToMimeTypeMap[FileExtension.FMF] = MimeType.VIDEO_X_ATOMIC3D_FEATURE;
    ExtensionToMimeTypeMap[FileExtension.FOR] = MimeType.TEXT_X_FORTRAN;
    ExtensionToMimeTypeMap[FileExtension.FPX] = MimeType.IMAGE_VND_FPX;
    ExtensionToMimeTypeMap[FileExtension.FRL] = MimeType.APPLICATION_FREELOADER;
    ExtensionToMimeTypeMap[FileExtension.FUNK] = MimeType.AUDIO_MAKE;
    ExtensionToMimeTypeMap[FileExtension.G] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.G3] = MimeType.IMAGE_G3FAX;
    ExtensionToMimeTypeMap[FileExtension.GIF] = MimeType.IMAGE_GIF;
    ExtensionToMimeTypeMap[FileExtension.GL] = MimeType.VIDEO_GL;
    ExtensionToMimeTypeMap[FileExtension.GLB] = MimeType.MODEL_GLTF_BINARY;
    ExtensionToMimeTypeMap[FileExtension.GLTF] = MimeType.MODEL_GLTF_JSON;
    ExtensionToMimeTypeMap[FileExtension.GSD] = MimeType.AUDIO_X_GSM;
    ExtensionToMimeTypeMap[FileExtension.GSM] = MimeType.AUDIO_X_GSM;
    ExtensionToMimeTypeMap[FileExtension.GSP] = MimeType.APPLICATION_X_GSP;
    ExtensionToMimeTypeMap[FileExtension.GSS] = MimeType.APPLICATION_X_GSS;
    ExtensionToMimeTypeMap[FileExtension.GTAR] = MimeType.APPLICATION_X_GTAR;
    ExtensionToMimeTypeMap[FileExtension.GZ] = MimeType.APPLICATION_X_COMPRESSED;
    ExtensionToMimeTypeMap[FileExtension.GZIP] = MimeType.MULTIPART_X_GZIP;
    ExtensionToMimeTypeMap[FileExtension.H] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.HDF] = MimeType.APPLICATION_X_HDF;
    ExtensionToMimeTypeMap[FileExtension.HELP] = MimeType.APPLICATION_X_HELPFILE;
    ExtensionToMimeTypeMap[FileExtension.HGL] = MimeType.APPLICATION_VND_HP_HPGL;
    ExtensionToMimeTypeMap[FileExtension.HH] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.HLB] = MimeType.TEXT_X_SCRIPT;
    ExtensionToMimeTypeMap[FileExtension.HLP] = MimeType.APPLICATION_HLP;
    ExtensionToMimeTypeMap[FileExtension.HPG] = MimeType.APPLICATION_VND_HP_HPGL;
    ExtensionToMimeTypeMap[FileExtension.HPGL] = MimeType.APPLICATION_VND_HP_HPGL;
    ExtensionToMimeTypeMap[FileExtension.HQX] = MimeType.APPLICATION_BINHEX;
    ExtensionToMimeTypeMap[FileExtension.HTA] = MimeType.APPLICATION_HTA;
    ExtensionToMimeTypeMap[FileExtension.HTC] = MimeType.TEXT_X_COMPONENT;
    ExtensionToMimeTypeMap[FileExtension.HTM] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.HTML] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.HTMLS] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.HTT] = MimeType.TEXT_WEBVIEWHTML;
    ExtensionToMimeTypeMap[FileExtension.HTX] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.ICE] = MimeType.X_CONFERENCE_X_COOLTALK;
    ExtensionToMimeTypeMap[FileExtension.ICO] = MimeType.IMAGE_X_ICON;
    ExtensionToMimeTypeMap[FileExtension.IDC] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.IEF] = MimeType.IMAGE_IEF;
    ExtensionToMimeTypeMap[FileExtension.IEFS] = MimeType.IMAGE_IEF;
    ExtensionToMimeTypeMap[FileExtension.IGES] = MimeType.MODEL_IGES;
    ExtensionToMimeTypeMap[FileExtension.IGS] = MimeType.MODEL_IGES;
    ExtensionToMimeTypeMap[FileExtension.IMA] = MimeType.APPLICATION_X_IMA;
    ExtensionToMimeTypeMap[FileExtension.IMAP] = MimeType.APPLICATION_X_HTTPD_IMAP;
    ExtensionToMimeTypeMap[FileExtension.INF] = MimeType.APPLICATION_INF;
    ExtensionToMimeTypeMap[FileExtension.INS] = MimeType.APPLICATION_X_INTERNETT_SIGNUP;
    ExtensionToMimeTypeMap[FileExtension.IP] = MimeType.APPLICATION_X_IP2;
    ExtensionToMimeTypeMap[FileExtension.ISU] = MimeType.VIDEO_X_ISVIDEO;
    ExtensionToMimeTypeMap[FileExtension.IT] = MimeType.AUDIO_IT;
    ExtensionToMimeTypeMap[FileExtension.IV] = MimeType.APPLICATION_X_INVENTOR;
    ExtensionToMimeTypeMap[FileExtension.IVR] = MimeType.I_WORLD_I_VRML;
    ExtensionToMimeTypeMap[FileExtension.IVY] = MimeType.APPLICATION_X_LIVESCREEN;
    ExtensionToMimeTypeMap[FileExtension.JAM] = MimeType.AUDIO_X_JAM;
    ExtensionToMimeTypeMap[FileExtension.JAV] = MimeType.TEXT_X_JAVA_SOURCE;
    ExtensionToMimeTypeMap[FileExtension.JAVA] = MimeType.TEXT_X_JAVA_SOURCE;
    ExtensionToMimeTypeMap[FileExtension.JCM] = MimeType.APPLICATION_X_JAVA_COMMERCE;
    ExtensionToMimeTypeMap[FileExtension.JFIF] = MimeType.IMAGE_JPEG;
    ExtensionToMimeTypeMap[FileExtension.JPE] = MimeType.IMAGE_JPEG;
    ExtensionToMimeTypeMap[FileExtension.JPEG] = MimeType.IMAGE_JPEG;
    ExtensionToMimeTypeMap[FileExtension.JPG] = MimeType.IMAGE_JPEG;
    ExtensionToMimeTypeMap[FileExtension.JPS] = MimeType.IMAGE_X_JPS;
    ExtensionToMimeTypeMap[FileExtension.JS] = MimeType.APPLICATION_JAVASCRIPT;
    ExtensionToMimeTypeMap[FileExtension.JUT] = MimeType.IMAGE_JUTVISION;
    ExtensionToMimeTypeMap[FileExtension.KAR] = MimeType.AUDIO_MIDI;
    ExtensionToMimeTypeMap[FileExtension.KSH] = MimeType.APPLICATION_X_KSH;
    ExtensionToMimeTypeMap[FileExtension.LA] = MimeType.AUDIO_NSPAUDIO;
    ExtensionToMimeTypeMap[FileExtension.LAM] = MimeType.AUDIO_X_LIVEAUDIO;
    ExtensionToMimeTypeMap[FileExtension.LATEX] = MimeType.APPLICATION_X_LATEX;
    ExtensionToMimeTypeMap[FileExtension.LHA] = MimeType.APPLICATION_LHA;
    ExtensionToMimeTypeMap[FileExtension.LHX] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.LIST] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.LMA] = MimeType.AUDIO_NSPAUDIO;
    ExtensionToMimeTypeMap[FileExtension.LOG] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.LSP] = MimeType.APPLICATION_X_LISP;
    ExtensionToMimeTypeMap[FileExtension.LST] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.LSX] = MimeType.TEXT_X_LA_ASF;
    ExtensionToMimeTypeMap[FileExtension.LTX] = MimeType.APPLICATION_X_LATEX;
    ExtensionToMimeTypeMap[FileExtension.LZH] = MimeType.APPLICATION_X_LZH;
    ExtensionToMimeTypeMap[FileExtension.LZX] = MimeType.APPLICATION_LZX;
    ExtensionToMimeTypeMap[FileExtension.M] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.M1V] = MimeType.VIDEO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.M2A] = MimeType.AUDIO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.M2V] = MimeType.VIDEO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.M3U] = MimeType.AUDIO_X_MPEQURL;
    ExtensionToMimeTypeMap[FileExtension.MAN] = MimeType.APPLICATION_X_TROFF_MAN;
    ExtensionToMimeTypeMap[FileExtension.MAP] = MimeType.APPLICATION_X_NAVIMAP;
    ExtensionToMimeTypeMap[FileExtension.MAR] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.MBD] = MimeType.APPLICATION_MBEDLET;
    ExtensionToMimeTypeMap[FileExtension.MC_DOLLAR] = MimeType.APPLICATION_X_MAGIC_CAP_PACKAGE_1_0;
    ExtensionToMimeTypeMap[FileExtension.MCD] = MimeType.APPLICATION_X_MATHCAD;
    ExtensionToMimeTypeMap[FileExtension.MCF] = MimeType.TEXT_MCF;
    ExtensionToMimeTypeMap[FileExtension.MCP] = MimeType.APPLICATION_NETMC;
    ExtensionToMimeTypeMap[FileExtension.ME] = MimeType.APPLICATION_X_TROFF_ME;
    ExtensionToMimeTypeMap[FileExtension.MHT] = MimeType.MESSAGE_RFC822;
    ExtensionToMimeTypeMap[FileExtension.MHTML] = MimeType.MESSAGE_RFC822;
    ExtensionToMimeTypeMap[FileExtension.MID] = MimeType.AUDIO_MIDI;
    ExtensionToMimeTypeMap[FileExtension.MIDI] = MimeType.AUDIO_MIDI;
    ExtensionToMimeTypeMap[FileExtension.MIF] = MimeType.APPLICATION_X_MIF;
    ExtensionToMimeTypeMap[FileExtension.MIME] = MimeType.WWW_MIME;
    ExtensionToMimeTypeMap[FileExtension.MJF] = MimeType.AUDIO_X_VND_AUDIOEXPLOSION_MJUICEMEDIAFILE;
    ExtensionToMimeTypeMap[FileExtension.MJPG] = MimeType.VIDEO_X_MOTION_JPEG;
    ExtensionToMimeTypeMap[FileExtension.MM] = MimeType.APPLICATION_BASE64;
    ExtensionToMimeTypeMap[FileExtension.MME] = MimeType.APPLICATION_BASE64;
    ExtensionToMimeTypeMap[FileExtension.MOD] = MimeType.AUDIO_MOD;
    ExtensionToMimeTypeMap[FileExtension.MOOV] = MimeType.VIDEO_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.MOV] = MimeType.VIDEO_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.MOVIE] = MimeType.VIDEO_X_SGI_MOVIE;
    ExtensionToMimeTypeMap[FileExtension.MP2] = MimeType.AUDIO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MP3] = MimeType.AUDIO_MPEG3;
    ExtensionToMimeTypeMap[FileExtension.MP4] = MimeType.VIDEO_MP4;
    ExtensionToMimeTypeMap[FileExtension.MPA] = MimeType.AUDIO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MPC] = MimeType.APPLICATION_X_PROJECT;
    ExtensionToMimeTypeMap[FileExtension.MPE] = MimeType.VIDEO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MPEG] = MimeType.VIDEO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MPG] = MimeType.VIDEO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MPGA] = MimeType.AUDIO_MPEG;
    ExtensionToMimeTypeMap[FileExtension.MPP] = MimeType.APPLICATION_VND_MS_PROJECT;
    ExtensionToMimeTypeMap[FileExtension.MPT] = MimeType.APPLICATION_X_PROJECT;
    ExtensionToMimeTypeMap[FileExtension.MPV] = MimeType.APPLICATION_X_PROJECT;
    ExtensionToMimeTypeMap[FileExtension.MPX] = MimeType.APPLICATION_X_PROJECT;
    ExtensionToMimeTypeMap[FileExtension.MRC] = MimeType.APPLICATION_MARC;
    ExtensionToMimeTypeMap[FileExtension.MS] = MimeType.APPLICATION_X_TROFF_MS;
    ExtensionToMimeTypeMap[FileExtension.MV] = MimeType.VIDEO_X_SGI_MOVIE;
    ExtensionToMimeTypeMap[FileExtension.MY] = MimeType.AUDIO_MAKE;
    ExtensionToMimeTypeMap[FileExtension.MZZ] = MimeType.APPLICATION_X_VND_AUDIOEXPLOSION_MZZ;
    ExtensionToMimeTypeMap[FileExtension.NAP] = MimeType.IMAGE_NAPLPS;
    ExtensionToMimeTypeMap[FileExtension.NAPLPS] = MimeType.IMAGE_NAPLPS;
    ExtensionToMimeTypeMap[FileExtension.NC] = MimeType.APPLICATION_X_NETCDF;
    ExtensionToMimeTypeMap[FileExtension.NCM] = MimeType.APPLICATION_VND_NOKIA_CONFIGURATION_MESSAGE;
    ExtensionToMimeTypeMap[FileExtension.NIF] = MimeType.IMAGE_X_NIFF;
    ExtensionToMimeTypeMap[FileExtension.NIFF] = MimeType.IMAGE_X_NIFF;
    ExtensionToMimeTypeMap[FileExtension.NIX] = MimeType.APPLICATION_X_MIX_TRANSFER;
    ExtensionToMimeTypeMap[FileExtension.NSC] = MimeType.APPLICATION_X_CONFERENCE;
    ExtensionToMimeTypeMap[FileExtension.NVD] = MimeType.APPLICATION_X_NAVIDOC;
    ExtensionToMimeTypeMap[FileExtension.O] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.ODA] = MimeType.APPLICATION_ODA;
    ExtensionToMimeTypeMap[FileExtension.OMC] = MimeType.APPLICATION_X_OMC;
    ExtensionToMimeTypeMap[FileExtension.OMCD] = MimeType.APPLICATION_X_OMCDATAMAKER;
    ExtensionToMimeTypeMap[FileExtension.OMCR] = MimeType.APPLICATION_X_OMCREGERATOR;
    ExtensionToMimeTypeMap[FileExtension.P] = MimeType.TEXT_X_PASCAL;
    ExtensionToMimeTypeMap[FileExtension.P10] = MimeType.APPLICATION_PKCS10;
    ExtensionToMimeTypeMap[FileExtension.P12] = MimeType.APPLICATION_PKCS_12;
    ExtensionToMimeTypeMap[FileExtension.P7A] = MimeType.APPLICATION_X_PKCS7_SIGNATURE;
    ExtensionToMimeTypeMap[FileExtension.P7C] = MimeType.APPLICATION_PKCS7_MIME;
    ExtensionToMimeTypeMap[FileExtension.P7M] = MimeType.APPLICATION_PKCS7_MIME;
    ExtensionToMimeTypeMap[FileExtension.P7R] = MimeType.APPLICATION_X_PKCS7_CERTREQRESP;
    ExtensionToMimeTypeMap[FileExtension.P7S] = MimeType.APPLICATION_PKCS7_SIGNATURE;
    ExtensionToMimeTypeMap[FileExtension.PART] = MimeType.APPLICATION_PRO_ENG;
    ExtensionToMimeTypeMap[FileExtension.PAS] = MimeType.TEXT_PASCAL;
    ExtensionToMimeTypeMap[FileExtension.PBM] = MimeType.IMAGE_X_PORDIV_BITMAP;
    ExtensionToMimeTypeMap[FileExtension.PCL] = MimeType.APPLICATION_X_PCL;
    ExtensionToMimeTypeMap[FileExtension.PCT] = MimeType.IMAGE_X_PICT;
    ExtensionToMimeTypeMap[FileExtension.PCX] = MimeType.IMAGE_X_PCX;
    ExtensionToMimeTypeMap[FileExtension.PDB] = MimeType.CHEMICAL_X_PDB;
    ExtensionToMimeTypeMap[FileExtension.PDF] = MimeType.APPLICATION_PDF;
    ExtensionToMimeTypeMap[FileExtension.PFUNK] = MimeType.AUDIO_MAKE;
    ExtensionToMimeTypeMap[FileExtension.PGM] = MimeType.IMAGE_X_PORDIV_GRAYMAP;
    ExtensionToMimeTypeMap[FileExtension.PIC] = MimeType.IMAGE_PICT;
    ExtensionToMimeTypeMap[FileExtension.PICT] = MimeType.IMAGE_PICT;
    ExtensionToMimeTypeMap[FileExtension.PKG] = MimeType.APPLICATION_X_NEWTON_COMPATIBLE_PKG;
    ExtensionToMimeTypeMap[FileExtension.PKO] = MimeType.APPLICATION_VND_MS_PKI_PKO;
    ExtensionToMimeTypeMap[FileExtension.PL] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.PLX] = MimeType.APPLICATION_X_PIXCLSCRIPT;
    ExtensionToMimeTypeMap[FileExtension.PM] = MimeType.IMAGE_X_XPIXMAP;
    ExtensionToMimeTypeMap[FileExtension.PM4] = MimeType.APPLICATION_X_PAGEMAKER;
    ExtensionToMimeTypeMap[FileExtension.PM5] = MimeType.APPLICATION_X_PAGEMAKER;
    ExtensionToMimeTypeMap[FileExtension.PNG] = MimeType.IMAGE_PNG;
    ExtensionToMimeTypeMap[FileExtension.PNM] = MimeType.IMAGE_X_PORDIV_ANYMAP;
    ExtensionToMimeTypeMap[FileExtension.POT] = MimeType.APPLICATION_MSPOWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.POV] = MimeType.MODEL_X_POV;
    ExtensionToMimeTypeMap[FileExtension.PPA] = MimeType.APPLICATION_VND_MS_POWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.PPM] = MimeType.IMAGE_X_PORDIV_PIXMAP;
    ExtensionToMimeTypeMap[FileExtension.PPS] = MimeType.APPLICATION_MSPOWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.PPT] = MimeType.APPLICATION_POWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.PPZ] = MimeType.APPLICATION_MSPOWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.PRE] = MimeType.APPLICATION_X_FREELANCE;
    ExtensionToMimeTypeMap[FileExtension.PRT] = MimeType.APPLICATION_PRO_ENG;
    ExtensionToMimeTypeMap[FileExtension.PS] = MimeType.APPLICATION_POSTSCRIPT;
    ExtensionToMimeTypeMap[FileExtension.PSD] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.PVU] = MimeType.PALEOVU_X_PV;
    ExtensionToMimeTypeMap[FileExtension.PWZ] = MimeType.APPLICATION_VND_MS_POWERPOINT;
    ExtensionToMimeTypeMap[FileExtension.PY] = MimeType.TEXT_X_SCRIPT_PHYTON;
    ExtensionToMimeTypeMap[FileExtension.PYC] = MimeType.APPLICATION_X_BYTECODE_PYTHON;
    ExtensionToMimeTypeMap[FileExtension.QCP] = MimeType.AUDIO_VND_QCELP;
    ExtensionToMimeTypeMap[FileExtension.QD3] = MimeType.X_WORLD_X_3DMF;
    ExtensionToMimeTypeMap[FileExtension.QD3D] = MimeType.X_WORLD_X_3DMF;
    ExtensionToMimeTypeMap[FileExtension.QIF] = MimeType.IMAGE_X_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.QT] = MimeType.VIDEO_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.QTC] = MimeType.VIDEO_X_QTC;
    ExtensionToMimeTypeMap[FileExtension.QTI] = MimeType.IMAGE_X_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.QTIF] = MimeType.IMAGE_X_QUICKTIME;
    ExtensionToMimeTypeMap[FileExtension.RA] = MimeType.AUDIO_X_REALAUDIO;
    ExtensionToMimeTypeMap[FileExtension.RAM] = MimeType.AUDIO_X_PN_REALAUDIO;
    ExtensionToMimeTypeMap[FileExtension.RAS] = MimeType.IMAGE_CMU_RASTER;
    ExtensionToMimeTypeMap[FileExtension.RAST] = MimeType.IMAGE_CMU_RASTER;
    ExtensionToMimeTypeMap[FileExtension.REXX] = MimeType.TEXT_X_SCRIPT_REXX;
    ExtensionToMimeTypeMap[FileExtension.RF] = MimeType.IMAGE_VND_RN_REALFLASH;
    ExtensionToMimeTypeMap[FileExtension.RGB] = MimeType.IMAGE_X_RGB;
    ExtensionToMimeTypeMap[FileExtension.RM] = MimeType.AUDIO_X_PN_REALAUDIO;
    ExtensionToMimeTypeMap[FileExtension.RMI] = MimeType.AUDIO_MID;
    ExtensionToMimeTypeMap[FileExtension.RMM] = MimeType.AUDIO_X_PN_REALAUDIO;
    ExtensionToMimeTypeMap[FileExtension.RMP] = MimeType.AUDIO_X_PN_REALAUDIO;
    ExtensionToMimeTypeMap[FileExtension.RNG] = MimeType.APPLICATION_RINGING_TONES;
    ExtensionToMimeTypeMap[FileExtension.RNX] = MimeType.APPLICATION_VND_RN_REALPLAYER;
    ExtensionToMimeTypeMap[FileExtension.ROFF] = MimeType.APPLICATION_X_TROFF;
    ExtensionToMimeTypeMap[FileExtension.RP] = MimeType.IMAGE_VND_RN_REALPIX;
    ExtensionToMimeTypeMap[FileExtension.RPM] = MimeType.AUDIO_X_PN_REALAUDIO_PLUGIN;
    ExtensionToMimeTypeMap[FileExtension.RT] = MimeType.TEXT_RICHTEXT;
    ExtensionToMimeTypeMap[FileExtension.RTF] = MimeType.APPLICATION_RTF;
    ExtensionToMimeTypeMap[FileExtension.RTX] = MimeType.APPLICATION_RTF;
    ExtensionToMimeTypeMap[FileExtension.RV] = MimeType.VIDEO_VND_RN_REALVIDEO;
    ExtensionToMimeTypeMap[FileExtension.S] = MimeType.TEXT_X_ASM;
    ExtensionToMimeTypeMap[FileExtension.S3M] = MimeType.AUDIO_S3M;
    ExtensionToMimeTypeMap[FileExtension.SAVEME] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.SBK] = MimeType.APPLICATION_X_TBOOK;
    ExtensionToMimeTypeMap[FileExtension.SCM] = MimeType.VIDEO_X_SCM;
    ExtensionToMimeTypeMap[FileExtension.SDML] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.SDP] = MimeType.APPLICATION_SDP;
    ExtensionToMimeTypeMap[FileExtension.SDR] = MimeType.APPLICATION_SOUNDER;
    ExtensionToMimeTypeMap[FileExtension.SEA] = MimeType.APPLICATION_SEA;
    ExtensionToMimeTypeMap[FileExtension.SET] = MimeType.APPLICATION_SET;
    ExtensionToMimeTypeMap[FileExtension.SGM] = MimeType.TEXT_SGML;
    ExtensionToMimeTypeMap[FileExtension.SGML] = MimeType.TEXT_SGML;
    ExtensionToMimeTypeMap[FileExtension.SH] = MimeType.APPLICATION_X_SH;
    ExtensionToMimeTypeMap[FileExtension.SHAR] = MimeType.APPLICATION_X_SHAR;
    ExtensionToMimeTypeMap[FileExtension.SHTML] = MimeType.TEXT_HTML;
    ExtensionToMimeTypeMap[FileExtension.SID] = MimeType.AUDIO_X_PSID;
    ExtensionToMimeTypeMap[FileExtension.SIT] = MimeType.APPLICATION_X_SIT;
    ExtensionToMimeTypeMap[FileExtension.SKD] = MimeType.APPLICATION_X_KOAN;
    ExtensionToMimeTypeMap[FileExtension.SKM] = MimeType.APPLICATION_X_KOAN;
    ExtensionToMimeTypeMap[FileExtension.SKP] = MimeType.APPLICATION_X_KOAN;
    ExtensionToMimeTypeMap[FileExtension.SKT] = MimeType.APPLICATION_X_KOAN;
    ExtensionToMimeTypeMap[FileExtension.SL] = MimeType.APPLICATION_X_SEELOGO;
    ExtensionToMimeTypeMap[FileExtension.SMI] = MimeType.APPLICATION_SMIL;
    ExtensionToMimeTypeMap[FileExtension.SMIL] = MimeType.APPLICATION_SMIL;
    ExtensionToMimeTypeMap[FileExtension.SND] = MimeType.AUDIO_BASIC;
    ExtensionToMimeTypeMap[FileExtension.SOL] = MimeType.APPLICATION_SOLIDS;
    ExtensionToMimeTypeMap[FileExtension.SPC] = MimeType.TEXT_X_SPEECH;
    ExtensionToMimeTypeMap[FileExtension.SPL] = MimeType.APPLICATION_FUTURESPLASH;
    ExtensionToMimeTypeMap[FileExtension.SPR] = MimeType.APPLICATION_X_SPRITE;
    ExtensionToMimeTypeMap[FileExtension.SPRITE] = MimeType.APPLICATION_X_SPRITE;
    ExtensionToMimeTypeMap[FileExtension.SRC] = MimeType.APPLICATION_X_WAIS_SOURCE;
    ExtensionToMimeTypeMap[FileExtension.SSI] = MimeType.TEXT_X_SERVER_PARSED_HTML;
    ExtensionToMimeTypeMap[FileExtension.SSM] = MimeType.APPLICATION_STREAMINGMEDIA;
    ExtensionToMimeTypeMap[FileExtension.SST] = MimeType.APPLICATION_VND_MS_PKI_CERTSTORE;
    ExtensionToMimeTypeMap[FileExtension.STEP] = MimeType.APPLICATION_STEP;
    ExtensionToMimeTypeMap[FileExtension.STL] = MimeType.APPLICATION_SLA;
    ExtensionToMimeTypeMap[FileExtension.STP] = MimeType.APPLICATION_STEP;
    ExtensionToMimeTypeMap[FileExtension.SV4CPIO] = MimeType.APPLICATION_X_SV4CPIO;
    ExtensionToMimeTypeMap[FileExtension.SV4CRC] = MimeType.APPLICATION_X_SV4CRC;
    ExtensionToMimeTypeMap[FileExtension.SVF] = MimeType.IMAGE_VND_DWG;
    ExtensionToMimeTypeMap[FileExtension.SVR] = MimeType.APPLICATION_X_WORLD;
    ExtensionToMimeTypeMap[FileExtension.SWF] = MimeType.APPLICATION_X_SHOCKWAVE_FLASH;
    ExtensionToMimeTypeMap[FileExtension.T] = MimeType.APPLICATION_X_TROFF;
    ExtensionToMimeTypeMap[FileExtension.TALK] = MimeType.TEXT_X_SPEECH;
    ExtensionToMimeTypeMap[FileExtension.TAR] = MimeType.APPLICATION_X_TAR;
    ExtensionToMimeTypeMap[FileExtension.TBK] = MimeType.APPLICATION_TOOLBOOK;
    ExtensionToMimeTypeMap[FileExtension.TCL] = MimeType.APPLICATION_X_TCL;
    ExtensionToMimeTypeMap[FileExtension.TCSH] = MimeType.TEXT_X_SCRIPT_TCSH;
    ExtensionToMimeTypeMap[FileExtension.TEX] = MimeType.APPLICATION_X_TEX;
    ExtensionToMimeTypeMap[FileExtension.TEXI] = MimeType.APPLICATION_X_TEXINFO;
    ExtensionToMimeTypeMap[FileExtension.TEXINFO] = MimeType.APPLICATION_X_TEXINFO;
    ExtensionToMimeTypeMap[FileExtension.TEXT] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.TGZ] = MimeType.APPLICATION_GNUTAR;
    ExtensionToMimeTypeMap[FileExtension.TIF] = MimeType.IMAGE_TIFF;
    ExtensionToMimeTypeMap[FileExtension.TIFF] = MimeType.IMAGE_TIFF;
    ExtensionToMimeTypeMap[FileExtension.TR] = MimeType.APPLICATION_X_TROFF;
    ExtensionToMimeTypeMap[FileExtension.TSI] = MimeType.AUDIO_TSP_AUDIO;
    ExtensionToMimeTypeMap[FileExtension.TSP] = MimeType.APPLICATION_DSPTYPE;
    ExtensionToMimeTypeMap[FileExtension.TSV] = MimeType.TEXT_TAB_SEPARATED_VALUES;
    ExtensionToMimeTypeMap[FileExtension.TURBOT] = MimeType.IMAGE_FLORIAN;
    ExtensionToMimeTypeMap[FileExtension.TXT] = MimeType.TEXT_PLAIN;
    ExtensionToMimeTypeMap[FileExtension.UIL] = MimeType.TEXT_X_UIL;
    ExtensionToMimeTypeMap[FileExtension.UNI] = MimeType.TEXT_URI_LIST;
    ExtensionToMimeTypeMap[FileExtension.UNIS] = MimeType.TEXT_URI_LIST;
    ExtensionToMimeTypeMap[FileExtension.UNV] = MimeType.APPLICATION_I_DEAS;
    ExtensionToMimeTypeMap[FileExtension.URI] = MimeType.TEXT_URI_LIST;
    ExtensionToMimeTypeMap[FileExtension.URIS] = MimeType.TEXT_URI_LIST;
    ExtensionToMimeTypeMap[FileExtension.USDZ] = MimeType.MODEL_VND_USDZ_ZIP;
    ExtensionToMimeTypeMap[FileExtension.USTAR] = MimeType.APPLICATION_X_USTAR;
    ExtensionToMimeTypeMap[FileExtension.UU] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.UUE] = MimeType.TEXT_X_UUENCODE;
    ExtensionToMimeTypeMap[FileExtension.VCD] = MimeType.APPLICATION_X_CDLINK;
    ExtensionToMimeTypeMap[FileExtension.VCS] = MimeType.TEXT_X_VCALENDAR;
    ExtensionToMimeTypeMap[FileExtension.VDA] = MimeType.APPLICATION_VDA;
    ExtensionToMimeTypeMap[FileExtension.VDO] = MimeType.VIDEO_VDO;
    ExtensionToMimeTypeMap[FileExtension.VEW] = MimeType.APPLICATION_GROUPWISE;
    ExtensionToMimeTypeMap[FileExtension.VIV] = MimeType.VIDEO_VIVO;
    ExtensionToMimeTypeMap[FileExtension.VIVO] = MimeType.VIDEO_VIVO;
    ExtensionToMimeTypeMap[FileExtension.VMD] = MimeType.APPLICATION_VOCALTEC_MEDIA_DESC;
    ExtensionToMimeTypeMap[FileExtension.VMF] = MimeType.APPLICATION_VOCALTEC_MEDIA_FILE;
    ExtensionToMimeTypeMap[FileExtension.VOC] = MimeType.AUDIO_VOC;
    ExtensionToMimeTypeMap[FileExtension.VOS] = MimeType.VIDEO_VOSAIC;
    ExtensionToMimeTypeMap[FileExtension.VOX] = MimeType.AUDIO_VOXWARE;
    ExtensionToMimeTypeMap[FileExtension.VQE] = MimeType.AUDIO_X_TWINVQ_PLUGIN;
    ExtensionToMimeTypeMap[FileExtension.VQF] = MimeType.AUDIO_X_TWINVQ;
    ExtensionToMimeTypeMap[FileExtension.VQL] = MimeType.AUDIO_X_TWINVQ_PLUGIN;
    ExtensionToMimeTypeMap[FileExtension.VRML] = MimeType.MODEL_VRML;
    ExtensionToMimeTypeMap[FileExtension.VRT] = MimeType.X_WORLD_X_VRT;
    ExtensionToMimeTypeMap[FileExtension.VSD] = MimeType.APPLICATION_X_VISIO;
    ExtensionToMimeTypeMap[FileExtension.VST] = MimeType.APPLICATION_X_VISIO;
    ExtensionToMimeTypeMap[FileExtension.VSW] = MimeType.APPLICATION_X_VISIO;
    ExtensionToMimeTypeMap[FileExtension.W60] = MimeType.APPLICATION_WORDPERFECT6_0;
    ExtensionToMimeTypeMap[FileExtension.W61] = MimeType.APPLICATION_WORDPERFECT6_1;
    ExtensionToMimeTypeMap[FileExtension.W6W] = MimeType.APPLICATION_MSWORD;
    ExtensionToMimeTypeMap[FileExtension.WAV] = MimeType.AUDIO_WAV;
    ExtensionToMimeTypeMap[FileExtension.WB1] = MimeType.APPLICATION_X_QPRO;
    ExtensionToMimeTypeMap[FileExtension.WBMP] = MimeType.IMAGE_VND_WAP_WBMP;
    ExtensionToMimeTypeMap[FileExtension.WEB] = MimeType.APPLICATION_VND_XARA;
    ExtensionToMimeTypeMap[FileExtension.WIZ] = MimeType.APPLICATION_MSWORD;
    ExtensionToMimeTypeMap[FileExtension.WK1] = MimeType.APPLICATION_X_123;
    ExtensionToMimeTypeMap[FileExtension.WMF] = MimeType.WINDOWS_METAFILE;
    ExtensionToMimeTypeMap[FileExtension.WML] = MimeType.TEXT_VND_WAP_WML;
    ExtensionToMimeTypeMap[FileExtension.WMLC] = MimeType.APPLICATION_VND_WAP_WMLC;
    ExtensionToMimeTypeMap[FileExtension.WMLS] = MimeType.TEXT_VND_WAP_WMLSCRIPT;
    ExtensionToMimeTypeMap[FileExtension.WMLSC] = MimeType.APPLICATION_VND_WAP_WMLSCRIPTC;
    ExtensionToMimeTypeMap[FileExtension.WORD] = MimeType.APPLICATION_MSWORD;
    ExtensionToMimeTypeMap[FileExtension.WP] = MimeType.APPLICATION_WORDPERFECT;
    ExtensionToMimeTypeMap[FileExtension.WP5] = MimeType.APPLICATION_WORDPERFECT;
    ExtensionToMimeTypeMap[FileExtension.WP6] = MimeType.APPLICATION_WORDPERFECT;
    ExtensionToMimeTypeMap[FileExtension.WPD] = MimeType.APPLICATION_WORDPERFECT;
    ExtensionToMimeTypeMap[FileExtension.WQ1] = MimeType.APPLICATION_X_LOTUS;
    ExtensionToMimeTypeMap[FileExtension.WRI] = MimeType.APPLICATION_MSWRITE;
    ExtensionToMimeTypeMap[FileExtension.WRL] = MimeType.APPLICATION_X_WORLD;
    ExtensionToMimeTypeMap[FileExtension.WRZ] = MimeType.MODEL_VRML;
    ExtensionToMimeTypeMap[FileExtension.WSC] = MimeType.TEXT_SCRIPLET;
    ExtensionToMimeTypeMap[FileExtension.WSRC] = MimeType.APPLICATION_X_WAIS_SOURCE;
    ExtensionToMimeTypeMap[FileExtension.WTK] = MimeType.APPLICATION_X_WINTALK;
    ExtensionToMimeTypeMap[FileExtension.XBM] = MimeType.IMAGE_X_XBITMAP;
    ExtensionToMimeTypeMap[FileExtension.XDR] = MimeType.VIDEO_X_AMT_DEMORUN;
    ExtensionToMimeTypeMap[FileExtension.XGZ] = MimeType.XGL_DRAWING;
    ExtensionToMimeTypeMap[FileExtension.XIF] = MimeType.IMAGE_VND_XIFF;
    ExtensionToMimeTypeMap[FileExtension.XL] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLA] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLB] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLC] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLD] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLK] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLL] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLM] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLS] = MimeType.APPLICATION_X_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLT] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLV] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XLW] = MimeType.APPLICATION_EXCEL;
    ExtensionToMimeTypeMap[FileExtension.XM] = MimeType.AUDIO_XM;
    ExtensionToMimeTypeMap[FileExtension.XML] = MimeType.APPLICATION_XML;
    ExtensionToMimeTypeMap[FileExtension.XPIX] = MimeType.APPLICATION_X_VND_LS_XPIX;
    ExtensionToMimeTypeMap[FileExtension.XPM] = MimeType.IMAGE_X_XPIXMAP;
    ExtensionToMimeTypeMap[FileExtension.X_PNG] = MimeType.IMAGE_PNG;
    ExtensionToMimeTypeMap[FileExtension.XSR] = MimeType.VIDEO_X_AMT_SHOWRUN;
    ExtensionToMimeTypeMap[FileExtension.XWD] = MimeType.IMAGE_X_XWD;
    ExtensionToMimeTypeMap[FileExtension.XYZ] = MimeType.CHEMICAL_X_PDB;
    ExtensionToMimeTypeMap[FileExtension.Z] = MimeType.APPLICATION_X_COMPRESSED;
    ExtensionToMimeTypeMap[FileExtension.ZIP] = MimeType.APPLICATION_ZIP;
    ExtensionToMimeTypeMap[FileExtension.ZOO] = MimeType.APPLICATION_OCTET_STREAM;
    ExtensionToMimeTypeMap[FileExtension.ZSH] = MimeType.TEXT_X_SCRIPT_ZSH;
}

} // namespace csp::common
