import { test, assert } from '../test_framework.js';
import { CSPFoundation, Common } from '../connected_spaces_platform.js';

test('MimeTypeHelperTests', 'BasicTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('some/file/path.jpg'), 'image/jpeg');
    assert.areEqual(helper.getMimeType('some/file/path.png'), 'image/png');
    assert.areEqual(helper.getMimeType('some/file/path.jpeg'), 'image/jpeg');
    assert.areEqual(helper.getMimeType('some/file/path.gltf'), 'model/gltf-json');
    assert.areEqual(helper.getMimeType('some/file/path.glb'), 'model/gltf-binary');
    assert.areEqual(helper.getMimeType('some/file/path.usdz'), 'model/vnd.usdz+zip');
    assert.areEqual(helper.getMimeType('some/file/path.zip'), 'application/zip');
    assert.areEqual(helper.getMimeType('some/file/path.unknown'), 'application/octet-stream');
});

test('MimeTypeHelperTests', 'UppercaseTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('SOME/FILE/PATH.JPG'), 'image/jpeg');
});

test('MimeTypeHelperTests', 'UnknownInputTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('some/path/to/a/file.unknown'), 'application/octet-stream');
});

test('MimeTypeHelperTests', 'EmptyInputTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType(''), 'application/octet-stream');
});

test('MimeTypeHelperTests', 'NoExtensionTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('path_with_no_extension'), 'application/octet-stream');
});

test('MimeTypeHelperTests', 'MultiplePeriodsTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('path.jpg.zip'), 'application/zip');
});

test('MimeTypeHelperTests', 'WhitespaceTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.getMimeType('path.jpg      \n   '), 'image/jpeg');
});

test('MimeTypeHelperTests', 'AccessMimeTypesTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.mimeType.iMAGE_JPEG, 'image/jpeg');
});

test('MimeTypeHelperTests', 'AccessMimetypesTest', async function() {
    const helper = Common.MimeTypeHelper.get();
    assert.areEqual(helper.fileExtension.jPG, 'jpg');
});
