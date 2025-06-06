#pragma once

#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

namespace oly
{

class OlympusFoundation;

}

namespace oly_common
{

class OLY_API MimeTypes
{
    /** @cond DO_NOT_DOCUMENT */
    friend class MimeTypeHelper;
    /** @endcond */

public:
    String APPLICATION_ACAD = "application/acad";
    String APPLICATION_ARJ = "application/arj";
    String APPLICATION_BASE64 = "application/base64";
    String APPLICATION_BINHEX = "application/binhex";
    String APPLICATION_BOOK = "application/book";
    String APPLICATION_CDF = "application/cdf";
    String APPLICATION_CLARISCAD = "application/clariscad";
    String APPLICATION_COMMONGROUND = "application/commonground";
    String APPLICATION_DRAFTING = "application/drafting";
    String APPLICATION_DSPTYPE = "application/dsptype";
    String APPLICATION_DXF = "application/dxf";
    String APPLICATION_ENVOY = "application/envoy";
    String APPLICATION_EXCEL = "application/excel";
    String APPLICATION_FREELOADER = "application/freeloader";
    String APPLICATION_FUTURESPLASH = "application/futuresplash";
    String APPLICATION_GNUTAR = "application/gnutar";
    String APPLICATION_GROUPWISE = "application/groupwise";
    String APPLICATION_HLP = "application/hlp";
    String APPLICATION_HTA = "application/hta";
    String APPLICATION_I_DEAS = "application/i-deas";
    String APPLICATION_INF = "application/inf";
    String APPLICATION_JAVA = "application/java";
    String APPLICATION_JAVASCRIPT = "application/javascript";
    String APPLICATION_LHA = "application/lha";
    String APPLICATION_LZX = "application/lzx";
    String APPLICATION_MARC = "application/marc";
    String APPLICATION_MBEDLET = "application/mbedlet";
    String APPLICATION_MIME = "application/mime";
    String APPLICATION_MSPOWERPOINT = "application/mspowerpoint";
    String APPLICATION_MSWORD = "application/msword";
    String APPLICATION_MSWRITE = "application/mswrite";
    String APPLICATION_NETMC = "application/netmc";
    String APPLICATION_OCTET_STREAM = "application/octet-stream";
    String APPLICATION_ODA = "application/oda";
    String APPLICATION_PDF = "application/pdf";
    String APPLICATION_PKCS10 = "application/pkcs10";
    String APPLICATION_PKCS7_MIME = "application/pkcs7-mime";
    String APPLICATION_PKCS7_SIGNATURE = "application/pkcs7-signature";
    String APPLICATION_PKCS_12 = "application/pkcs-12";
    String APPLICATION_PKCS_CRL = "application/pkcs-crl";
    String APPLICATION_PKIX_CERT = "application/pkix-cert";
    String APPLICATION_POSTSCRIPT = "application/postscript";
    String APPLICATION_POWERPOINT = "application/powerpoint";
    String APPLICATION_PRO_ENG = "application/pro_eng";
    String APPLICATION_RINGING_TONES = "application/ringing-tones";
    String APPLICATION_RTF = "application/rtf";
    String APPLICATION_SDP = "application/sdp";
    String APPLICATION_SEA = "application/sea";
    String APPLICATION_SET = "application/set";
    String APPLICATION_SLA = "application/sla";
    String APPLICATION_SMIL = "application/smil";
    String APPLICATION_SOLIDS = "application/solids";
    String APPLICATION_SOUNDER = "application/sounder";
    String APPLICATION_STEP = "application/step";
    String APPLICATION_STREAMINGMEDIA = "application/streamingmedia";
    String APPLICATION_TOOLBOOK = "application/toolbook";
    String APPLICATION_VDA = "application/vda";
    String APPLICATION_VND_FDF = "application/vnd.fdf";
    String APPLICATION_VND_HP_HPGL = "application/vnd.hp-hpgl";
    String APPLICATION_VND_MS_PKI_CERTSTORE = "application/vnd.ms-pki.certstore";
    String APPLICATION_VND_MS_PKI_PKO = "application/vnd.ms-pki.pko";
    String APPLICATION_VND_MS_PKI_SECCAT = "application/vnd.ms-pki.seccat";
    String APPLICATION_VND_MS_POWERPOINT = "application/vnd.ms-powerpoint";
    String APPLICATION_VND_MS_PROJECT = "application/vnd.ms-project";
    String APPLICATION_VND_NOKIA_CONFIGURATION_MESSAGE = "application/vnd.nokia.configuration-message";
    String APPLICATION_VND_RN_REALPLAYER = "application/vnd.rn-realplayer";
    String APPLICATION_VND_WAP_WMLC = "application/vnd.wap.wmlc";
    String APPLICATION_VND_WAP_WMLSCRIPTC = "application/vnd.wap.wmlscriptc";
    String APPLICATION_VND_XARA = "application/vnd.xara";
    String APPLICATION_VOCALTEC_MEDIA_DESC = "application/vocaltec-media-desc";
    String APPLICATION_VOCALTEC_MEDIA_FILE = "application/vocaltec-media-file";
    String APPLICATION_WORDPERFECT = "application/wordperfect";
    String APPLICATION_WORDPERFECT6_0 = "application/wordperfect6.0";
    String APPLICATION_WORDPERFECT6_1 = "application/wordperfect6.1";
    String APPLICATION_X_123 = "application/x-123";
    String APPLICATION_X_AIM = "application/x-aim";
    String APPLICATION_X_AUTHORWARE_BIN = "application/x-authorware-bin";
    String APPLICATION_X_AUTHORWARE_MAPZ = "application/x-authorware-mapz";
    String APPLICATION_X_AUTHORWARE_SEG = "application/x-authorware-seg";
    String APPLICATION_X_BCPIO = "application/x-bcpio";
    String APPLICATION_X_BSH = "application/x-bsh";
    String APPLICATION_X_BYTECODE_PYTHON = "application/x-bytecode.python";
    String APPLICATION_X_BZIP = "application/x-bzip";
    String APPLICATION_X_BZIP2 = "application/x-bzip2";
    String APPLICATION_X_CDLINK = "application/x-cdlink";
    String APPLICATION_X_CHAT = "application/x-chat";
    String APPLICATION_X_COCOA = "application/x-cocoa";
    String APPLICATION_X_COMPRESSED = "application/x-compressed";
    String APPLICATION_X_CONFERENCE = "application/x-conference";
    String APPLICATION_X_CPIO = "application/x-cpio";
    String APPLICATION_X_CPT = "application/x-cpt";
    String APPLICATION_X_CSH = "application/x-csh";
    String APPLICATION_X_DEEPV = "application/x-deepv";
    String APPLICATION_X_DIRECTOR = "application/x-director";
    String APPLICATION_X_DVI = "application/x-dvi";
    String APPLICATION_X_ELC = "application/x-elc";
    String APPLICATION_X_ENVOY = "application/x-envoy";
    String APPLICATION_X_ESREHBER = "application/x-esrehber";
    String APPLICATION_X_EXCEL = "application/x-excel";
    String APPLICATION_X_FREELANCE = "application/x-freelance";
    String APPLICATION_X_GSP = "application/x-gsp";
    String APPLICATION_X_GSS = "application/x-gss";
    String APPLICATION_X_GTAR = "application/x-gtar";
    String APPLICATION_X_HDF = "application/x-hdf";
    String APPLICATION_X_HELPFILE = "application/x-helpfile";
    String APPLICATION_X_HTTPD_IMAP = "application/x-httpd-imap";
    String APPLICATION_X_IMA = "application/x-ima";
    String APPLICATION_X_INTERNETT_SIGNUP = "application/x-internett-signup";
    String APPLICATION_X_INVENTOR = "application/x-inventor";
    String APPLICATION_X_IP2 = "application/x-ip2";
    String APPLICATION_X_JAVA_COMMERCE = "application/x-java-commerce";
    String APPLICATION_X_KOAN = "application/x-koan";
    String APPLICATION_X_KSH = "application/x-ksh";
    String APPLICATION_X_LATEX = "application/x-latex";
    String APPLICATION_X_LISP = "application/x-lisp";
    String APPLICATION_X_LIVESCREEN = "application/x-livescreen";
    String APPLICATION_X_LOTUS = "application/x-lotus";
    String APPLICATION_X_LOTUSSCREENCAM = "application/x-lotusscreencam";
    String APPLICATION_X_LZH = "application/x-lzh";
    String APPLICATION_X_MAGIC_CAP_PACKAGE_1_0 = "application/x-magic-cap-package-1.0";
    String APPLICATION_X_MATHCAD = "application/x-mathcad";
    String APPLICATION_X_MIF = "application/x-mif";
    String APPLICATION_X_MIX_TRANSFER = "application/x-mix-transfer";
    String APPLICATION_X_MPLAYER2 = "application/x-mplayer2";
    String APPLICATION_X_NAVI_ANIMATION = "application/x-navi-animation";
    String APPLICATION_X_NAVIDOC = "application/x-navidoc";
    String APPLICATION_X_NAVIMAP = "application/x-navimap";
    String APPLICATION_X_NETCDF = "application/x-netcdf";
    String APPLICATION_X_NEWTON_COMPATIBLE_PKG = "application/x-newton-compatible-pkg";
    String APPLICATION_X_NOKIA_9000_COMMUNICATOR_ADD_ON_SOFTWARE = "application/x-nokia-9000-communicator-add-on-software";
    String APPLICATION_X_OMC = "application/x-omc";
    String APPLICATION_X_OMCDATAMAKER = "application/x-omcdatamaker";
    String APPLICATION_X_OMCREGERATOR = "application/x-omcregerator";
    String APPLICATION_X_PAGEMAKER = "application/x-pagemaker";
    String APPLICATION_X_PCL = "application/x-pcl";
    String APPLICATION_X_PIXCLSCRIPT = "application/x-pixclscript";
    String APPLICATION_X_PKCS7_CERTREQRESP = "application/x-pkcs7-certreqresp";
    String APPLICATION_X_PKCS7_SIGNATURE = "application/x-pkcs7-signature";
    String APPLICATION_X_POINTPLUS = "application/x-pointplus";
    String APPLICATION_X_PROJECT = "application/x-project";
    String APPLICATION_X_QPRO = "application/x-qpro";
    String APPLICATION_X_SEELOGO = "application/x-seelogo";
    String APPLICATION_X_SH = "application/x-sh";
    String APPLICATION_X_SHAR = "application/x-shar";
    String APPLICATION_X_SHOCKWAVE_FLASH = "application/x-shockwave-flash";
    String APPLICATION_X_SIT = "application/x-sit";
    String APPLICATION_X_SPRITE = "application/x-sprite";
    String APPLICATION_X_SV4CPIO = "application/x-sv4cpio";
    String APPLICATION_X_SV4CRC = "application/x-sv4crc";
    String APPLICATION_X_TAR = "application/x-tar";
    String APPLICATION_X_TBOOK = "application/x-tbook";
    String APPLICATION_X_TCL = "application/x-tcl";
    String APPLICATION_X_TEX = "application/x-tex";
    String APPLICATION_X_TEXINFO = "application/x-texinfo";
    String APPLICATION_X_TROFF = "application/x-troff";
    String APPLICATION_X_TROFF_MAN = "application/x-troff-man";
    String APPLICATION_X_TROFF_ME = "application/x-troff-me";
    String APPLICATION_X_TROFF_MS = "application/x-troff-ms";
    String APPLICATION_X_USTAR = "application/x-ustar";
    String APPLICATION_X_VISIO = "application/x-visio";
    String APPLICATION_X_VND_AUDIOEXPLOSION_MZZ = "application/x-vnd.audioexplosion.mzz";
    String APPLICATION_X_VND_LS_XPIX = "application/x-vnd.ls-xpix";
    String APPLICATION_X_WAIS_SOURCE = "application/x-wais-source";
    String APPLICATION_X_WINTALK = "application/x-wintalk";
    String APPLICATION_X_WORLD = "application/x-world";
    String APPLICATION_X_X509_CA_CERT = "application/x-x509-ca-cert";
    String APPLICATION_XML = "application/xml";
    String APPLICATION_ZIP = "application/zip";
    String AUDIO_AIFF = "audio/aiff";
    String AUDIO_BASIC = "audio/basic";
    String AUDIO_IT = "audio/it";
    String AUDIO_MAKE = "audio/make";
    String AUDIO_MID = "audio/mid";
    String AUDIO_MIDI = "audio/midi";
    String AUDIO_MOD = "audio/mod";
    String AUDIO_MPEG = "audio/mpeg";
    String AUDIO_MPEG3 = "audio/mpeg3";
    String AUDIO_NSPAUDIO = "audio/nspaudio";
    String AUDIO_S3M = "audio/s3m";
    String AUDIO_TSP_AUDIO = "audio/tsp-audio";
    String AUDIO_VND_QCELP = "audio/vnd.qcelp";
    String AUDIO_VOC = "audio/voc";
    String AUDIO_VOXWARE = "audio/voxware";
    String AUDIO_WAV = "audio/wav";
    String AUDIO_X_GSM = "audio/x-gsm";
    String AUDIO_X_JAM = "audio/x-jam";
    String AUDIO_X_LIVEAUDIO = "audio/x-liveaudio";
    String AUDIO_X_MPEQURL = "audio/x-mpequrl";
    String AUDIO_X_PN_REALAUDIO = "audio/x-pn-realaudio";
    String AUDIO_X_PN_REALAUDIO_PLUGIN = "audio/x-pn-realaudio-plugin";
    String AUDIO_X_PSID = "audio/x-psid";
    String AUDIO_X_REALAUDIO = "audio/x-realaudio";
    String AUDIO_X_TWINVQ = "audio/x-twinvq";
    String AUDIO_X_TWINVQ_PLUGIN = "audio/x-twinvq-plugin";
    String AUDIO_X_VND_AUDIOEXPLOSION_MJUICEMEDIAFILE = "audio/x-vnd.audioexplosion.mjuicemediafile";
    String AUDIO_XM = "audio/xm";
    String CHEMICAL_X_PDB = "chemical/x-pdb";
    String I_WORLD_I_VRML = "i-world/i-vrml";
    String IMAGE_BMP = "image/bmp";
    String IMAGE_CMU_RASTER = "image/cmu-raster";
    String IMAGE_FIF = "image/fif";
    String IMAGE_FLORIAN = "image/florian";
    String IMAGE_G3FAX = "image/g3fax";
    String IMAGE_GIF = "image/gif";
    String IMAGE_IEF = "image/ief";
    String IMAGE_JPEG = "image/jpeg";
    String IMAGE_JUTVISION = "image/jutvision";
    String IMAGE_NAPLPS = "image/naplps";
    String IMAGE_PICT = "image/pict";
    String IMAGE_PNG = "image/png";
    String IMAGE_TIFF = "image/tiff";
    String IMAGE_VND_DWG = "image/vnd.dwg";
    String IMAGE_VND_FPX = "image/vnd.fpx";
    String IMAGE_VND_RN_REALFLASH = "image/vnd.rn-realflash";
    String IMAGE_VND_RN_REALPIX = "image/vnd.rn-realpix";
    String IMAGE_VND_WAP_WBMP = "image/vnd.wap.wbmp";
    String IMAGE_VND_XIFF = "image/vnd.xiff";
    String IMAGE_X_ICON = "image/x-icon";
    String IMAGE_X_JG = "image/x-jg";
    String IMAGE_X_JPS = "image/x-jps";
    String IMAGE_X_NIFF = "image/x-niff";
    String IMAGE_X_PCX = "image/x-pcx";
    String IMAGE_X_PICT = "image/x-pict";
    String IMAGE_X_PORDIV_ANYMAP = "image/x-pordiv-anymap";
    String IMAGE_X_PORDIV_BITMAP = "image/x-pordiv-bitmap";
    String IMAGE_X_PORDIV_GRAYMAP = "image/x-pordiv-graymap";
    String IMAGE_X_PORDIV_PIXMAP = "image/x-pordiv-pixmap";
    String IMAGE_X_QUICKTIME = "image/x-quicktime";
    String IMAGE_X_RGB = "image/x-rgb";
    String IMAGE_X_XBITMAP = "image/x-xbitmap";
    String IMAGE_X_XPIXMAP = "image/x-xpixmap";
    String IMAGE_X_XWD = "image/x-xwd";
    String MESSAGE_RFC822 = "message/rfc822";
    String MODEL_GLTF_BINARY = "model/gltf-binary";
    String MODEL_GLTF_JSON = "model/gltf-json";
    String MODEL_IGES = "model/iges";
    String MODEL_VND_DWF = "model/vnd.dwf";
    String MODEL_VND_USDZ_ZIP = "model/vnd.usdz+zip";
    String MODEL_VRML = "model/vrml";
    String MODEL_X_POV = "model/x-pov";
    String MULTIPART_X_GZIP = "multipart/x-gzip";
    String PALEOVU_X_PV = "paleovu/x-pv";
    String TEXT_ASP = "text/asp";
    String TEXT_HTML = "text/html";
    String TEXT_MCF = "text/mcf";
    String TEXT_PASCAL = "text/pascal";
    String TEXT_PLAIN = "text/plain";
    String TEXT_RICHTEXT = "text/richtext";
    String TEXT_SCRIPLET = "text/scriplet";
    String TEXT_SGML = "text/sgml";
    String TEXT_TAB_SEPARATED_VALUES = "text/tab-separated-values";
    String TEXT_URI_LIST = "text/uri-list";
    String TEXT_VND_ABC = "text/vnd.abc";
    String TEXT_VND_FMI_FLEXSTOR = "text/vnd.fmi.flexstor";
    String TEXT_VND_WAP_WML = "text/vnd.wap.wml";
    String TEXT_VND_WAP_WMLSCRIPT = "text/vnd.wap.wmlscript";
    String TEXT_WEBVIEWHTML = "text/webviewhtml";
    String TEXT_X_ASM = "text/x-asm";
    String TEXT_X_AUDIOSOFT_INTRA = "text/x-audiosoft-intra";
    String TEXT_X_C = "text/x-c";
    String TEXT_X_COMPONENT = "text/x-component";
    String TEXT_X_FORTRAN = "text/x-fortran";
    String TEXT_X_JAVA_SOURCE = "text/x-java-source";
    String TEXT_X_LA_ASF = "text/x-la-asf";
    String TEXT_X_PASCAL = "text/x-pascal";
    String TEXT_X_SCRIPT = "text/x-script";
    String TEXT_X_SCRIPT_ELISP = "text/x-script.elisp";
    String TEXT_X_SCRIPT_PHYTON = "text/x-script.phyton";
    String TEXT_X_SCRIPT_REXX = "text/x-script.rexx";
    String TEXT_X_SCRIPT_TCSH = "text/x-script.tcsh";
    String TEXT_X_SCRIPT_ZSH = "text/x-script.zsh";
    String TEXT_X_SERVER_PARSED_HTML = "text/x-server-parsed-html";
    String TEXT_X_SETEXT = "text/x-setext";
    String TEXT_X_SPEECH = "text/x-speech";
    String TEXT_X_UIL = "text/x-uil";
    String TEXT_X_UUENCODE = "text/x-uuencode";
    String TEXT_X_VCALENDAR = "text/x-vcalendar";
    String VIDEO_ANIMAFLEX = "video/animaflex";
    String VIDEO_AVI = "video/avi";
    String VIDEO_AVS_VIDEO = "video/avs-video";
    String VIDEO_DL = "video/dl";
    String VIDEO_FLI = "video/fli";
    String VIDEO_GL = "video/gl";
    String VIDEO_MP4 = "video/mp4";
    String VIDEO_MPEG = "video/mpeg";
    String VIDEO_QUICKTIME = "video/quicktime";
    String VIDEO_VDO = "video/vdo";
    String VIDEO_VIVO = "video/vivo";
    String VIDEO_VND_RN_REALVIDEO = "video/vnd.rn-realvideo";
    String VIDEO_VOSAIC = "video/vosaic";
    String VIDEO_X_AMT_DEMORUN = "video/x-amt-demorun";
    String VIDEO_X_AMT_SHOWRUN = "video/x-amt-showrun";
    String VIDEO_X_ATOMIC3D_FEATURE = "video/x-atomic3d-feature";
    String VIDEO_X_DV = "video/x-dv";
    String VIDEO_X_ISVIDEO = "video/x-isvideo";
    String VIDEO_X_MOTION_JPEG = "video/x-motion-jpeg";
    String VIDEO_X_MS_ASF = "video/x-ms-asf";
    String VIDEO_X_QTC = "video/x-qtc";
    String VIDEO_X_SCM = "video/x-scm";
    String VIDEO_X_SGI_MOVIE = "video/x-sgi-movie";
    String WINDOWS_METAFILE = "windows/metafile";
    String WWW_MIME = "www/mime";
    String X_CONFERENCE_X_COOLTALK = "x-conference/x-cooltalk";
    String X_WORLD_X_3DMF = "x-world/x-3dmf";
    String X_WORLD_X_VRT = "x-world/x-vrt";
    String XGL_DRAWING = "xgl/drawing";

private:
    MimeTypes();
};

class OLY_API FileExtensions
{
    /** @cond DO_NOT_DOCUMENT */
    friend class MimeTypeHelper;
    /** @endcond */
public:
    String _3DM = "3dm";
    String _3DMF = "3dmf";
    String A = "a";
    String AAB = "aab";
    String AAM = "aam";
    String AAS = "aas";
    String ABC = "abc";
    String ACGI = "acgi";
    String AFL = "afl";
    String AI = "ai";
    String AIF = "aif";
    String AIFC = "aifc";
    String AIFF = "aiff";
    String AIM = "aim";
    String AIP = "aip";
    String ANI = "ani";
    String AOS = "aos";
    String APS = "aps";
    String ARC = "arc";
    String ARJ = "arj";
    String ART = "art";
    String ASF = "asf";
    String ASM = "asm";
    String ASP = "asp";
    String ASX = "asx";
    String AU = "au";
    String AVI = "avi";
    String AVS = "avs";
    String BCPIO = "bcpio";
    String BIN = "bin";
    String BM = "bm";
    String BMP = "bmp";
    String BOO = "boo";
    String BOOK = "book";
    String BOZ = "boz";
    String BSH = "bsh";
    String BZ = "bz";
    String BZ2 = "bz2";
    String C = "c";
    String C_PLUS_PLUS = "c++";
    String CAT = "cat";
    String CC = "cc";
    String CCAD = "ccad";
    String CCO = "cco";
    String CDF = "cdf";
    String CER = "cer";
    String CHA = "cha";
    String CHAT = "chat";
    String CLASS = "class";
    String COM = "com";
    String CONF = "conf";
    String CPIO = "cpio";
    String CPP = "cpp";
    String CPT = "cpt";
    String CRL = "crl";
    String CRT = "crt";
    String CSH = "csh";
    String CSS = "css";
    String CXX = "cxx";
    String DCR = "dcr";
    String DEEPV = "deepv";
    String DEF = "def";
    String DER = "der";
    String DIF = "dif";
    String DIR = "dir";
    String DL = "dl";
    String DOC = "doc";
    String DOT = "dot";
    String DP = "dp";
    String DRW = "drw";
    String DUMP = "dump";
    String DV = "dv";
    String DVI = "dvi";
    String DWF = "dwf";
    String DWG = "dwg";
    String DXF = "dxf";
    String EL = "el";
    String ELC = "elc";
    String ENV = "env";
    String EPS = "eps";
    String ES = "es";
    String ETX = "etx";
    String EVY = "evy";
    String EXE = "exe";
    String F = "f";
    String F77 = "f77";
    String F90 = "f90";
    String FDF = "fdf";
    String FIF = "fif";
    String FLI = "fli";
    String FLO = "flo";
    String FLX = "flx";
    String FMF = "fmf";
    String FOR = "for";
    String FPX = "fpx";
    String FRL = "frl";
    String FUNK = "funk";
    String G = "g";
    String G3 = "g3";
    String GIF = "gif";
    String GL = "gl";
    String GLB = "glb";
    String GLTF = "gltf";
    String GSD = "gsd";
    String GSM = "gsm";
    String GSP = "gsp";
    String GSS = "gss";
    String GTAR = "gtar";
    String GZ = "gz";
    String GZIP = "gzip";
    String H = "h";
    String HDF = "hdf";
    String HELP = "help";
    String HGL = "hgl";
    String HH = "hh";
    String HLB = "hlb";
    String HLP = "hlp";
    String HPG = "hpg";
    String HPGL = "hpgl";
    String HQX = "hqx";
    String HTA = "hta";
    String HTC = "htc";
    String HTM = "htm";
    String HTML = "html";
    String HTMLS = "htmls";
    String HTT = "htt";
    String HTX = "htx";
    String ICE = "ice";
    String ICO = "ico";
    String IDC = "idc";
    String IEF = "ief";
    String IEFS = "iefs";
    String IGES = "iges";
    String IGS = "igs";
    String IMA = "ima";
    String IMAP = "imap";
    String INF = "inf";
    String INS = "ins";
    String IP = "ip";
    String ISU = "isu";
    String IT = "it";
    String IV = "iv";
    String IVR = "ivr";
    String IVY = "ivy";
    String JAM = "jam";
    String JAV = "jav";
    String JAVA = "java";
    String JCM = "jcm";
    String JFIF = "jfif";
    String JPE = "jpe";
    String JPEG = "jpeg";
    String JPG = "jpg";
    String JPS = "jps";
    String JS = "js";
    String JUT = "jut";
    String KAR = "kar";
    String KSH = "ksh";
    String LA = "la";
    String LAM = "lam";
    String LATEX = "latex";
    String LHA = "lha";
    String LHX = "lhx";
    String LIST = "list";
    String LMA = "lma";
    String LOG = "log";
    String LSP = "lsp";
    String LST = "lst";
    String LSX = "lsx";
    String LTX = "ltx";
    String LZH = "lzh";
    String LZX = "lzx";
    String M = "m";
    String M1V = "m1v";
    String M2A = "m2a";
    String M2V = "m2v";
    String M3U = "m3u";
    String MAN = "man";
    String MAP = "map";
    String MAR = "mar";
    String MBD = "mbd";
    String MC_DOLLAR = "mc$";
    String MCD = "mcd";
    String MCF = "mcf";
    String MCP = "mcp";
    String ME = "me";
    String MHT = "mht";
    String MHTML = "mhtml";
    String MID = "mid";
    String MIDI = "midi";
    String MIF = "mif";
    String MIME = "mime";
    String MJF = "mjf";
    String MJPG = "mjpg";
    String MM = "mm";
    String MME = "mme";
    String MOD = "mod";
    String MOOV = "moov";
    String MOV = "mov";
    String MOVIE = "movie";
    String MP2 = "mp2";
    String MP3 = "mp3";
    String MP4 = "mp4";
    String MPA = "mpa";
    String MPC = "mpc";
    String MPE = "mpe";
    String MPEG = "mpeg";
    String MPG = "mpg";
    String MPGA = "mpga";
    String MPP = "mpp";
    String MPT = "mpt";
    String MPV = "mpv";
    String MPX = "mpx";
    String MRC = "mrc";
    String MS = "ms";
    String MV = "mv";
    String MY = "my";
    String MZZ = "mzz";
    String NAP = "nap";
    String NAPLPS = "naplps";
    String NC = "nc";
    String NCM = "ncm";
    String NIF = "nif";
    String NIFF = "niff";
    String NIX = "nix";
    String NSC = "nsc";
    String NVD = "nvd";
    String O = "o";
    String ODA = "oda";
    String OMC = "omc";
    String OMCD = "omcd";
    String OMCR = "omcr";
    String P = "p";
    String P10 = "p10";
    String P12 = "p12";
    String P7A = "p7a";
    String P7C = "p7c";
    String P7M = "p7m";
    String P7R = "p7r";
    String P7S = "p7s";
    String PART = "part";
    String PAS = "pas";
    String PBM = "pbm";
    String PCL = "pcl";
    String PCT = "pct";
    String PCX = "pcx";
    String PDB = "pdb";
    String PDF = "pdf";
    String PFUNK = "pfunk";
    String PGM = "pgm";
    String PIC = "pic";
    String PICT = "pict";
    String PKG = "pkg";
    String PKO = "pko";
    String PL = "pl";
    String PLX = "plx";
    String PM = "pm";
    String PM4 = "pm4";
    String PM5 = "pm5";
    String PNG = "png";
    String PNM = "pnm";
    String POT = "pot";
    String POV = "pov";
    String PPA = "ppa";
    String PPM = "ppm";
    String PPS = "pps";
    String PPT = "ppt";
    String PPZ = "ppz";
    String PRE = "pre";
    String PRT = "prt";
    String PS = "ps";
    String PSD = "psd";
    String PVU = "pvu";
    String PWZ = "pwz";
    String PY = "py";
    String PYC = "pyc";
    String QCP = "qcp";
    String QD3 = "qd3";
    String QD3D = "qd3d";
    String QIF = "qif";
    String QT = "qt";
    String QTC = "qtc";
    String QTI = "qti";
    String QTIF = "qtif";
    String RA = "ra";
    String RAM = "ram";
    String RAS = "ras";
    String RAST = "rast";
    String REXX = "rexx";
    String RF = "rf";
    String RGB = "rgb";
    String RM = "rm";
    String RMI = "rmi";
    String RMM = "rmm";
    String RMP = "rmp";
    String RNG = "rng";
    String RNX = "rnx";
    String ROFF = "roff";
    String RP = "rp";
    String RPM = "rpm";
    String RT = "rt";
    String RTF = "rtf";
    String RTX = "rtx";
    String RV = "rv";
    String S = "s";
    String S3M = "s3m";
    String SAVEME = "saveme";
    String SBK = "sbk";
    String SCM = "scm";
    String SDML = "sdml";
    String SDP = "sdp";
    String SDR = "sdr";
    String SEA = "sea";
    String SET = "set";
    String SGM = "sgm";
    String SGML = "sgml";
    String SH = "sh";
    String SHAR = "shar";
    String SHTML = "shtml";
    String SID = "sid";
    String SIT = "sit";
    String SKD = "skd";
    String SKM = "skm";
    String SKP = "skp";
    String SKT = "skt";
    String SL = "sl";
    String SMI = "smi";
    String SMIL = "smil";
    String SND = "snd";
    String SOL = "sol";
    String SPC = "spc";
    String SPL = "spl";
    String SPR = "spr";
    String SPRITE = "sprite";
    String SRC = "src";
    String SSI = "ssi";
    String SSM = "ssm";
    String SST = "sst";
    String STEP = "step";
    String STL = "stl";
    String STP = "stp";
    String SV4CPIO = "sv4cpio";
    String SV4CRC = "sv4crc";
    String SVF = "svf";
    String SVR = "svr";
    String SWF = "swf";
    String T = "t";
    String TALK = "talk";
    String TAR = "tar";
    String TBK = "tbk";
    String TCL = "tcl";
    String TCSH = "tcsh";
    String TEX = "tex";
    String TEXI = "texi";
    String TEXINFO = "texinfo";
    String TEXT = "text";
    String TGZ = "tgz";
    String TIF = "tif";
    String TIFF = "tiff";
    String TR = "tr";
    String TSI = "tsi";
    String TSP = "tsp";
    String TSV = "tsv";
    String TURBOT = "turbot";
    String TXT = "txt";
    String UIL = "uil";
    String UNI = "uni";
    String UNIS = "unis";
    String UNV = "unv";
    String URI = "uri";
    String URIS = "uris";
    String USDZ = "usdz";
    String USTAR = "ustar";
    String UU = "uu";
    String UUE = "uue";
    String VCD = "vcd";
    String VCS = "vcs";
    String VDA = "vda";
    String VDO = "vdo";
    String VEW = "vew";
    String VIV = "viv";
    String VIVO = "vivo";
    String VMD = "vmd";
    String VMF = "vmf";
    String VOC = "voc";
    String VOS = "vos";
    String VOX = "vox";
    String VQE = "vqe";
    String VQF = "vqf";
    String VQL = "vql";
    String VRML = "vrml";
    String VRT = "vrt";
    String VSD = "vsd";
    String VST = "vst";
    String VSW = "vsw";
    String W60 = "w60";
    String W61 = "w61";
    String W6W = "w6w";
    String WAV = "wav";
    String WB1 = "wb1";
    String WBMP = "wbmp";
    String WEB = "web";
    String WIZ = "wiz";
    String WK1 = "wk1";
    String WMF = "wmf";
    String WML = "wml";
    String WMLC = "wmlc";
    String WMLS = "wmls";
    String WMLSC = "wmlsc";
    String WORD = "word";
    String WP = "wp";
    String WP5 = "wp5";
    String WP6 = "wp6";
    String WPD = "wpd";
    String WQ1 = "wq1";
    String WRI = "wri";
    String WRL = "wrl";
    String WRZ = "wrz";
    String WSC = "wsc";
    String WSRC = "wsrc";
    String WTK = "wtk";
    String XBM = "xbm";
    String XDR = "xdr";
    String XGZ = "xgz";
    String XIF = "xif";
    String XL = "xl";
    String XLA = "xla";
    String XLB = "xlb";
    String XLC = "xlc";
    String XLD = "xld";
    String XLK = "xlk";
    String XLL = "xll";
    String XLM = "xlm";
    String XLS = "xls";
    String XLT = "xlt";
    String XLV = "xlv";
    String XLW = "xlw";
    String XM = "xm";
    String XML = "xml";
    String XPIX = "xpix";
    String XPM = "xpm";
    String X_PNG = "x-png";
    String XSR = "xsr";
    String XWD = "xwd";
    String XYZ = "xyz";
    String Z = "z";
    String ZIP = "zip";
    String ZOO = "zoo";
    String ZSH = "zsh";

private:
    FileExtensions();
};

class OLY_API MimeTypeHelper
{
public:
    static MimeTypeHelper& Get();

    MimeTypes MimeType;
    FileExtensions FileExtension;

    String& GetMimeType(const String& FilePath);

private:
    MimeTypeHelper();

    String Default = MimeType.APPLICATION_OCTET_STREAM;

    Map<String, String> ExtensionToMimeTypeMap;
};

} // namespace oly_common
