import {
  commonArrayToJSArray,
  dataUrlToFile,
  fileToBuffer,
  jsArrayToCommonArrayofAssetCollectionType,
  jsArrayToCommonArrayofString,
  resultState as resultStatus,
  testImage,
} from "./helpers.js";
import {
  ClientUserAgent,
  Common,
  CSPFoundation,
  freeBuffer,
  Multiplayer,
  ready,
  Systems,
} from "./node_modules/connected-spaces-platform.web/connectedspacesplatform.js";

// Magnopus Services Endpoint to connect to.
const ENDPOINT = "https://ogs.magnopus-stg.cloud";

// Tenant defines the application scope.
const TENANT = "CSP_HELLO_WORLD";

// Desired email / password combination to run the tests.
const EMAIL = "";
const PASSWORD = "";

// If you wish to connect to a specific space, you can define its ID here.
let SPACE_ID = "";

const runAllExamples = () => {
  // Track if we have a supplied space, or if we need to create one.
  const createAndRemoveSpace = SPACE_ID === "";

  // Ensure credentials are set.
  if (PASSWORD === "" || EMAIL === "") {
    console.log("Please set your email and desired password in index.js");
    return;
  }

  // If we should use debug version of Connected Spaces Platform (CSP).
  const debug = false;

  // We call ready as promise, as to know CSP is ready.
  ready(debug).then(async () => {
    // Create CSP user agent information.
    const userAgent = ClientUserAgent.create();
    userAgent.clientSKU = "csp-javascript-examples";
    const agent = navigator.userAgent.toLowerCase();
    userAgent.clientOS = agent.split(/[()]/)[1];
    userAgent.olympusVersion = CSPFoundation.getBuildID();

    // Initialise CSP against a given endpoint and tenant
    CSPFoundation.initialise(ENDPOINT, TENANT);

    // Set the user agent constructed above.
    CSPFoundation.setClientUserAgentInfo(userAgent);

    // Get CSP systems.
    const systemsManager = Systems.SystemsManager.get();
    const userSystem = systemsManager.getUserSystem();
    const spaceSystem = systemsManager.getSpaceSystem();
    const assetSystem = systemsManager.getAssetSystem();
    const spaceEntitySystem = systemsManager.getSpaceEntitySystem();
    const graphQLSystem = systemsManager.getGraphQLSystem();

    // For CSP to process updates, we can call `Tick` and a given rate.
    setInterval(() => {
      CSPFoundation.tick();
    }, 1000 / 30); // 30 FPS

    // Attempt signup
    const signupResult = await userSystem.createUser(
      null,
      null,
      EMAIL,
      PASSWORD,
      false,
      true,
      null,
      null
    );
    if (signupResult.getResultCode() === Systems.EResultCode.Success) {
      console.log(
        "Signup successful, please check your email and re-run this example when verified."
      );
      return;
    }
    signupResult.delete();

    // Attempt login
    const loginResult = await userSystem.login("", EMAIL, PASSWORD);
    if (loginResult.getLoginState().state !== Systems.ELoginState.LoggedIn) {
      console.log(
        "Login unsuccessful, please check your credentials are correct and your email address verified."
      );
      return;
    }

    console.log("login successful");
    loginResult.delete();

    // Create Space if no space is defined.
    if (createAndRemoveSpace) {
      const spaceResult = await spaceSystem.createSpace(
        "Test Space",
        "",
        Systems.SpaceAttributes.Private,
        null,
        Common.Map.ofStringAndString(),
        null
      );

      if (spaceResult.getResultCode() === Systems.EResultCode.Success) {
        SPACE_ID = spaceResult.getSpace().id;
        console.log("created space", SPACE_ID);
      }
      spaceResult.delete();
    }

    // Getting space details
    const spaceResult = await spaceSystem.getSpace(SPACE_ID);
    console.log(
      "Get Space Details",
      resultStatus(spaceResult),
      spaceResult.getSpace().name
    );
    spaceResult.delete();

    // Create an asset collection associated with our space
    const collectionResult = await assetSystem.createAssetCollection(
      SPACE_ID,
      null,
      `test_upload-${Math.floor(Math.random() * Math.random() * 100000)}`,
      null,
      Systems.EAssetCollectionType.DEFAULT,
      null
    );

    console.log("Created Collection", resultStatus(collectionResult));
    const collection = collectionResult.getAssetCollection();

    // Create a detail
    const detailResult = await assetSystem.createAsset(
      collection,
      "test-image.png",
      null,
      null,
      Systems.EAssetType.IMAGE
    );
    console.log("Create Detail", resultStatus(detailResult));

    // Get the associated detail.
    const detail = detailResult.getAsset();

    // Not sure why this line is required.
    detail.fileName = "test-image.png";

    // Our test file object.
    const file = await dataUrlToFile(testImage, "test-image.png");

    // Convert file to buffer
    const buffer = await fileToBuffer(file);

    // Upload a file to the asset detail.
    const uploadResult = await assetSystem.uploadAssetData(
      collection,
      detail,
      buffer,
      (requestProgress, responseProgress) => {}
    );
    console.log("File upload complete", resultStatus(uploadResult));
    detailResult.delete();
    uploadResult.delete();
    collectionResult.delete();
    // Free buffer
    freeBuffer(buffer.buffer);

    // Retrieve asset
    const assetListResult = await assetSystem.findAssetCollections(
      null,
      null,
      null,
      jsArrayToCommonArrayofAssetCollectionType([
        Systems.EAssetCollectionType.DEFAULT,
      ]),
      null,
      jsArrayToCommonArrayofString([SPACE_ID]),
      null,
      null
    );

    for (const asset of commonArrayToJSArray(
      assetListResult.getAssetCollections()
    )) {
      console.log("Asset found", asset.name);
    }
    assetListResult.delete();

    // Delete asset
    const deleteResult = await assetSystem.deleteAsset(collection, detail);
    console.log("Asset deleted", resultStatus(deleteResult));
    deleteResult.delete();

    // Get all available spaces for current user
    const availableSpaces = await spaceSystem.getSpaces();
    console.log("Spaces available to user", resultStatus(availableSpaces));

    for (const space of commonArrayToJSArray(availableSpaces.getSpaces())) {
      console.log("Space found", space.name);
    }

    // Run a query via graphQL with a partial name filter to search.
    const searchTerm = "Test";
    const graphQLResult = await graphQLSystem.runQuery(`spaces(
      pagination: { limit: 10, skip: 0 }
      filters: { partialName: "${searchTerm}"}
    ) { 
      itemTotalCount, 
      items{
          groupId,
          name,
          description
      }
    }`);

    // Parse JSON response.
    const spaceSearchRes = JSON.parse(graphQLResult.getResponse());
    console.log(
      "GraphQL space search: found ",
      spaceSearchRes.data.spaces.itemTotalCount,
      " spaces that match 'test'."
    );
    graphQLResult.delete();

    // Set up callbacks for entity events,
    // first we listen to when entities are created
    spaceEntitySystem.setEntityCreatedCallback((newEntity) => {
      console.log("New Entity", newEntity.getId());

      // We can also listen to updates for this new entity
      newEntity.setUpdateCallback((entity, updateFlags, componentUpdates) => {
        if (
          updateFlags & Multiplayer.SpaceEntityUpdateFlags.UPDATE_FLAGS_POSITION
        ) {
          console.log(
            "Received new position for Entity",
            entity.getId(),
            entity.getPosition()
          );
        }
      });

      // And when a entity is destroyed in multiplayer.
      newEntity.setDestroyCallback(() => {
        console.log("Entity destroyed", newEntity.getId());
      });
    });

    // Enter space
    const enterSpaceResult = await spaceSystem.enterSpace(SPACE_ID, false);
    console.log("Enter Space", resultStatus(enterSpaceResult));
    enterSpaceResult.delete();

    // Creating an avatar
    const myEntity = await spaceEntitySystem.createAvatar(
      "name",
      Multiplayer.SpaceTransform.create(),
      Multiplayer.AvatarState.Idle,
      "id",
      Multiplayer.AvatarPlayMode.Default
    );

    console.log("Avatar created", myEntity.getId());

    // Listening to an updates on our avatar the same way we do above.
    myEntity.setUpdateCallback((entity, updateFlags, componentUpdates) => {
      if (
        updateFlags & Multiplayer.SpaceEntityUpdateFlags.UPDATE_FLAGS_POSITION
      ) {
        const position = entity.getPosition();
        console.log(
          "Avatar position updated",
          entity.getId(),
          position.x,
          position.y,
          position.z
        );
      }
    });
    // Updating the avatar's position

    // Get previous position
    const position = myEntity.getPosition();
    // Update x and y based on mouse position
    position.x = position.x + 1;
    position.y = position.y + 2;
    // Update entity position
    myEntity.setPosition(position);
    // Queue entity for update.
    spaceEntitySystem.queueEntityUpdate(myEntity);

    // After a few seconds we can disconnect, delete the space and logout.
    setTimeout(async () => {
      const exitSpaceResult = await spaceSystem.exitSpace();
      console.log("Exit Space", resultStatus(exitSpaceResult));
      exitSpaceResult.delete();

      // Delete this space
      if (createAndRemoveSpace) {
        const deleteResult = await spaceSystem.deleteSpace(SPACE_ID);
        console.log("Deleted Space", resultStatus(deleteResult));
        deleteResult.delete();
      }

      // Logout
      const logoutResult = await userSystem.logout();
      console.log("Successfully logged out", resultStatus(logoutResult));
      logoutResult.delete();

      // Shutdown
      CSPFoundation.shutdown();
      console.log("CSP shutdown");
    }, 3000);
  });
};

runAllExamples();
