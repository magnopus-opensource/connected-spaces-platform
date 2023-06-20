using Common = Csp.Common;

using CSharpTests;

namespace CSPEngine
{
	class MimeTypeHelperTests
	{
#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_BASIC_TEST
		[Test]
		public static void BasicTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("some/file/path.png"), "image/png");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.jpg"), "image/jpeg");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.jpeg"), "image/jpeg");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.gltf"), "model/gltf-json");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.glb"), "model/gltf-binary");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.usdz"), "model/vnd.usdz+zip");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.zip"), "application/zip");
			Assert.AreEqual(Helper.GetMimeType("some/file/path.unknown"), "application/octet-stream");
		}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_UPPERCASE_TEST
		[Test]
		public static void UppercaseTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("SOME/FILE/PATH.JPG"), "image/jpeg");
		}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_WITH_UNKNOWN_INPUT_TEST
		public static void UnknownInputTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("some/path/to/a/file.unknown"), "application/octet-stream");
		}
#endif


#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_WITH_EMPTY_INPUT_TEST
		[Test]
		public static void EmptyInputTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType(""), "application/octet-stream");
		}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_NO_EXTENSION_TEST
		[Test]
		public static void NoExtensionTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("path_with_no_extension"), "application/octet-stream");
		}

#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_MULTIPLE_PERIODS_TEST
		[Test]
		public static void MultiplePeriodsTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("path.jpg.zip"), "application/zip");
		}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_WHITESPACE_TEST
		[Test]
		public static void WhitespaceTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.GetMimeType("path.jpg      \n   "), "image/jpeg");
		}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_ACCESS_MIMETYPES_TEST
		[Test]
		public static void AccessMimeTypesTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.MimeType.IMAGE_JPEG, "image/jpeg");
		}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_MIMETYPEHELPER_TESTS || RUN_MIMETYPEHELPER_ACCESS_FILEEXTENSIONS_TEST
		[Test]
		public static void AccessFileExtensionsTest()
		{
			var Helper = Common.MimeTypeHelper.Get();
			Assert.AreEqual(Helper.FileExtension.JPG, "jpg");
		}
#endif
	}
}
