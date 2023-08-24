import { assert, pushCleanupFunction } from '../test_framework.js';
import { commonArrayToJSArray, jsArrayToCommonArray } from '../conversion_helpers.js';

import { uint8ArrayToBuffer, Common, Systems } from '../connected_spaces_platform.js';


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem 
 * @param {!Systems.AssetCollection} assetCollection
 * @param {!boolean} [deleteFoundationResources] 
 */
 export async function deleteAssetCollection(assetSystem, assetCollection, deleteFoundationResources = true) {
    const assetCollectionId = assetCollection.id;
    const result = await assetSystem.deleteAssetCollection(assetCollection);
    
    assert.succeeded(result);

    result.delete();

    console.debug(`AssetCollection deleted (Id: ${assetCollectionId})`);

    if (deleteFoundationResources)
        assetCollection.delete();
}


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem 
 * @param {?Systems.Space} space 
 * @param {?string} parentId 
 * @param {!string} name 
 * @param {?Systems.EAssetCollectionType} type 
 * @param {?string[]} [tags] 
 * @param {!boolean} [pushCleanup] 
 * @param {!boolean} [deleteFoundationResources] 
 * @returns {Promise<!Systems.AssetCollection>} the created assetCollection
 */
 export async function createAssetCollection(assetSystem, space = null, parentId = null, name, type = null, tags = null, pushCleanup = true, deleteFoundationResources = true) {
    let _type = type ?? Systems.EAssetCollectionType.DEFAULT;
    let /** @type {Common.Array<string>} */ _tags = jsArrayToCommonArray(tags, String);
    
    // the .ts wrapper is not currently able to set asset collection tags or type
    const result = await assetSystem.createAssetCollection(space?.id, parentId, name, null, _type, _tags);
    
    assert.succeeded(result);

    const assetCollection = result.getAssetCollection();
    console.debug(`AssetCollection created (Id: ${assetCollection.id}, Name: ${assetCollection.name}, Tag: ${assetCollection.tags[0]})`);

    result.delete();

    if (pushCleanup) {
        pushCleanupFunction(async () => {
            await deleteAssetCollection(assetSystem, assetCollection, deleteFoundationResources);
        });
    }

    return assetCollection
}


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem
 * @param {!Systems.AssetCollection} collection
 * @param {!Systems.Asset} asset
 * @param {!boolean} [deleteFoundationResources] 
 */
 export async function deleteAsset(assetSystem, collection, asset, deleteFoundationResources = true) {
    const assetId = asset.id;
    const result = await assetSystem.deleteAsset(collection, asset);
    
    assert.succeeded(result);

    result.delete();

    console.debug(`Asset deleted (Id: ${assetId}, Collection Id: ${collection.id})`);

    if (deleteFoundationResources)
        asset.delete();
}


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem 
 * @param {!Systems.AssetCollection} assetCollection 
 * @param {?string} name 
 * @param {!string} thirdPartyPackagedAssetIdentifier
 * @param {!boolean} [pushCleanup] 
 * @param {!boolean} [deleteFoundationResources] 
 * @returns {Promise<!Systems.Asset>} the created asset
 */
export async function createAsset(assetSystem, assetCollection, name, thirdPartyPackagedAssetIdentifier = null, thirdPartyPlatformType = null,  pushCleanup = true, deleteFoundationResources = true)
{
    const result = await assetSystem.createAsset(assetCollection, name, thirdPartyPackagedAssetIdentifier, thirdPartyPlatformType, Systems.EAssetType.MODEL);

    assert.succeeded(result);

    const asset = result.getAsset();
    console.debug(`Asset created (Id: ${asset.id}, Collection Id: ${asset.assetCollectionId})`);

    result.delete();

    if (pushCleanup) {
        pushCleanupFunction(async () => {
            await deleteAsset(assetSystem, assetCollection, asset, deleteFoundationResources);
        });
    }

    return asset;
}


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem 
 * @param {!Systems.AssetCollection} assetCollection
 * @param {!Systems.Asset} asset
 * @param {!Systems.AssetDataSource} source 
 * @returns {Promise<!string>} the created asset Uri
 */
export async function uploadAssetData(assetSystem, assetCollection, asset, source) {
    const result = await assetSystem.uploadAssetData(assetCollection, asset, source, (_rq, _rs)=>{});

    assert.succeeded(result);

    const uri = result.getUri();
    result.delete();

    return uri;
}


/**
 * 
 * @param {!string} localFilePath 
 * @param {!string} mimeType
 * @returns {Promise<!Systems.BufferAssetDataSource>} the created BufferAssetDataSource
 */
export async function createBufferAssetDataSource(localFilePath, mimeType) {
    var source = Systems.BufferAssetDataSource.create();
    const filePath = window.location.host + localFilePath;

    let response = await fetch('http://' + filePath,{
        method: 'get',
    });

    // Receive blob
    let json = await response.blob();
    const arrayBuffer = await json.arrayBuffer();
    const rawData = new Uint8Array(arrayBuffer, 0, json.size);
    source.bufferLength = rawData.buffer.byteLength;
    
    // Convert uint8 to buffer
    source.buffer = uint8ArrayToBuffer(rawData);
    source.setMimeType(mimeType);

    return source;
}


/**
 * 
 * @param {!Systems.AssetSystem} assetSystem 
 * @param {?Systems.Space} space 
 * @param {?string} parentId 
 * @param {?Systems.EAssetCollectionType} type 
 * @param {?string[]} [tags] 
 * @param {?string[]} [names] 
 * @param {?number} resultsSkipNumber
 * @param {?number} resultsMaxNumber
 * @returns {Promise<!Systems.AssetCollection[]>}
 */
 export async function getAssetCollectionsByCriteria(assetSystem, space = null, parentId = null, type = null, tags = null, names = null, resultsSkipNumber = null, resultsMaxNumber = null) {
    let /** @type {Common.Array<string>} */ _tags = jsArrayToCommonArray(tags, String);
    let /** @type {Common.Array<string>} */ _names = jsArrayToCommonArray(names, String);

    const result = await assetSystem.getAssetCollectionsByCriteria(space?.id, parentId, type, _tags, _names, resultsSkipNumber, resultsMaxNumber);

    assert.succeeded(result);

    const assetCollections = result.getAssetCollections();
    result.delete();

    const _assetCollections = commonArrayToJSArray(assetCollections);
    assetCollections.delete();

    return _assetCollections;
}

export async function getAssetsByCriteria(assetSystem, assetCollectionIds, assetIds = null, assetNames = null, assetTypes = null) {
    let /** @type {Common.Array<string>} */ _assetIds = jsArrayToCommonArray(assetIds, String);
    let /** @type {Common.Array<string>} */ _assetNames = jsArrayToCommonArray(assetNames, String);
    let /** @type {Common.Array<string>} */ _assetCollectionIds = jsArrayToCommonArray(assetCollectionIds, String);

    const result = await assetSystem.getAssetsByCriteria(_assetCollectionIds, _assetIds, _assetNames, assetTypes);

    assert.succeeded(result);

    const assets = result.getAssets();
    result.delete();

    const _assets = commonArrayToJSArray(assets);
    assets.delete();

    return _assets;
}