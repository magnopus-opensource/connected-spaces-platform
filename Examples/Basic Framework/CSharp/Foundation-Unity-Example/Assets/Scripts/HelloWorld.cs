// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Csp;
using Csp.Multiplayer;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using FoundationCommon = Csp.Common;
using FoundationSystems = Csp.Systems;
using FoundationAsset = Csp.Systems.Asset;
using UnityEngine;
using UnityEngine.Windows;

public class HelloWorld : MonoBehaviour
{
    [SerializeField] private AccountUI accountUI;

    private const string endPointUri = "https://ogs-ostage.magnoboard.com";
    private const string TenantKey = "FOUNDATION_HELLO_WORLD";
    private const string defaultSpaceSite = "Void";
    private const int TickDelayMilliseconds = 1000 / 60; //60fps
    private readonly string exampleAssetName = "example.png";
    private readonly string exampleAssetPath = "Assets/StreamingAssets/";
    private bool foundationHasStarted;
    private bool enteredSpace;
    private FoundationSystems.UserSystem userSystem;
    private FoundationSystems.SpaceSystem spaceSystem;
    private FoundationSystems.AssetSystem assetSystem;
    private FoundationSystems.GraphQLSystem graphQLSystem;
    private SpaceEntitySystem entitySystem;
    private MultiplayerConnection connection;
    private CancellationTokenSource cancellationTokenSource;
    private FoundationSystems.Space createdSpace;

    // Initialisation of Foundation

    #region Initialisation

    private void Awake()
    {
        var userAgent = new ClientUserAgent()
        {
            CHSEnvironment = "OStage",
            ClientEnvironment = "Stage",
            ClientOS = SystemInfo.operatingSystem,
            ClientSKU = "foundation-cSharp-examples",
            ClientVersion = "0.0.2",
            CSPVersion = CSPFoundation.GetBuildID()
        };

        Application.quitting += QuitFoundation;

        accountUI.OnSignIn += SignInAsync;
        accountUI.OnSignUp += SignUpAsync;
        accountUI.OnEnterSpace += EnterSpaceAsync;
        accountUI.OnCreateSpace += CreateSpaceAsync;
        accountUI.OnExitSpace += ExitSpaceAsync;
        accountUI.OnDeleteSpace += DeleteSpaceAsync;

        StartFoundation(endPointUri, TenantKey, userAgent);
    }

    /// <summary>
    /// Starts the underlying Foundation systems, you should also
    /// call <see cref="StopFoundation"/> during the consuming application's shutdown process.
    /// </summary>
    /// <param name="backendEndpoint">The endpoint url for the backend services.</param>
    /// <param name="tenant">The assigned Tenant value for this application.</param>
    /// <param name="userAgent">Identifiable information to this application client.</param>
    private void StartFoundation(string backendEndpoint, string tenant, ClientUserAgent userAgent)
    {
        Debug.Log("Initializing Foundation ...");
        bool successInit = CSPFoundation.Initialise(backendEndpoint, tenant);
        if (!successInit)
        {
            Debug.Log("Failed to initialize Olympus Foundation. Error is within Foundation package.");
            return;
        }

        CSPFoundation.SetClientUserAgentInfo(userAgent);
        Debug.Log("Initialized Foundation.");

        foundationHasStarted = true;
        InitializeSystems();
    }

    /// <summary>
    /// Starts a cancellable forever-loop to call Tick with a specified delay.
    /// Tick is necessary for multiplayer. It allows sending object messages and patches.
    /// It can also fail / throw an exception.
    /// </summary>
    private void StartTickLoop()
    {
        cancellationTokenSource?.Cancel();
        cancellationTokenSource?.Dispose();
        cancellationTokenSource = new CancellationTokenSource();

        TickAsync(cancellationTokenSource.Token);
    }

    /// <summary>
    /// Stops the Tick loop.
    /// </summary>
    private void StopTickLoop()
    {
        cancellationTokenSource?.Cancel();
    }

    private async void TickAsync(CancellationToken cancellationToken)
    {
        while (true)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                cancellationTokenSource.Dispose();
                cancellationTokenSource = null;
                return;
            }

            CSPFoundation.Tick();

            await Task.Delay(TickDelayMilliseconds);
        }
    }

    /// <summary>
    /// Initializes required systems
    /// </summary>
    private void InitializeSystems()
    {
        userSystem = FoundationSystems.SystemsManager.Get().GetUserSystem();
        spaceSystem = FoundationSystems.SystemsManager.Get().GetSpaceSystem();
        assetSystem = FoundationSystems.SystemsManager.Get().GetAssetSystem();
        graphQLSystem = FoundationSystems.SystemsManager.Get().GetGraphQLSystem();
    }

    /// <summary>
    /// Shuts down the underlying Foundation systems, this should be called by the consuming application
    /// during application shutdown once all of the dependant systems have been shutdown.
    /// </summary>
    private void StopFoundation()
    {
        Debug.Log("Shutting down Olympus Foundation ...");
        bool successShutdown = CSPFoundation.Shutdown();
        if (!successShutdown)
        {
            Debug.Log("Failed to shut down Foundation. Error is within Foundation package.");
            return;
        }

        Debug.Log("Shutdown Olympus Foundation");
        foundationHasStarted = false;
    }

    private void QuitFoundation()
    {
        Application.quitting -= QuitFoundation;

        if (foundationHasStarted)
        {
            if (enteredSpace)
            {
                ExitSpaceAsync();
            }

            StopFoundation();
        }
    }

    #endregion

    // Authentication / Login

    #region Authentication & Flow

    /// <summary>
    /// Listener to trigger creating a new account based on UI input for the user using the given email and password.
    /// </summary>
    /// <param name="email"> Email of the new account. </param>
    /// <param name="password"> Password of the new account. </param>
    private async void SignUpAsync(string email, string password)
    {
        await CreateUserAsync(email, password);
    }

    /// <summary>
    /// Listener to trigger signing in with an account based on UI input for the user using the given email and password.
    /// </summary>
    /// <param name="email"> Email of the new account. </param>
    /// <param name="password"> Password of the new account. </param>
    private async void SignInAsync(string email, string password)
    {
        await LoginAsync(email, password);
        await SearchSpacesAsync();
        await SearchSpacesUsingGraphQLAsync();
    }

    /// <summary>
    /// Requests the Foundation layer to create a new account for the user using the given email and password.
    /// </summary>
    /// <param name="email"> email of the new account.</param>
    /// <param name="password"> password of the new account.</param>
    /// <returns> Just the Task object to await.</returns>
    private async Task CreateUserAsync(string email, string password)
    {
        Debug.Log($"Creating account with Email {email} ...");

        using FoundationSystems.ProfileResult result =
            await userSystem.CreateUser(null, null, email, password, false, null, null);
        FoundationSystems.Profile profile = result.GetProfile();

        Debug.Log($"Created account with Email {profile.Email}. Please check your email to verify your account.");
    }

    /// <summary>
    /// Requests the Foundation layer to Login with email and password.
    /// </summary>
    /// <param name="email"> Email of the user. </param>
    /// <param name="password"> Password of the user. </param>
    /// <returns> Just the Task object to await.</returns>
    private async Task LoginAsync(string email, string password)
    {
        Debug.Log($"Logging in with Email {email} ...");

        using FoundationSystems.LoginStateResult loginResult = await userSystem.Login(string.Empty, email, password);

        Debug.Log($"Logged in with Email {email}.");
    }

    /// <summary>
    /// Requests the Foundation layer to make the user logout.
    /// Not used for this example as shutting down the Foundation layer will automatically log the user out.
    /// </summary>
    /// <returns> Just the Task object to await.</returns>
    private async Task LogoutAsync()
    {
        Debug.Log("Logging out ...");
        await userSystem.Logout();
    }

    #endregion

    // Spaces - creating, joining, deleting a space

    #region Spaces

    /// <summary>
    /// Creates a simple space with the provided name.
    /// </summary>
    /// <param name="spaceName">The name of the space</param>
    private async void CreateSpaceAsync(string spaceName)
    {
        Debug.Log($"Creating space: {spaceName} ...");

        var metadata = new Dictionary<string, string>()
        {
            { "site", defaultSpaceSite }
        };
        using FoundationSystems.SpaceResult spaceResult = await spaceSystem.CreateSpace(spaceName, string.Empty,
            FoundationSystems.SpaceAttributes.Private, null, ToFoundationMap(metadata), null);

        createdSpace = spaceResult.GetSpace();
        Debug.Log($"Created space with name: {createdSpace.Name} and ID: {createdSpace.Id}");
    }

    /// <summary>
    /// Enters the space with the given spaceId. If the space was just created, the
    /// useCreatedSpace is used to enter that space instead.
    /// </summary>
    /// <param name="spaceId">The destination space's ID.</param>
    /// <param name="useCreatedSpace">Whether the user wants to enter the space they just created instead of another specified one via the spaceId parameter.</param>
    private async void EnterSpaceAsync(string spaceId, bool useCreatedSpace)
    {
        string targetSpaceId;

        if (useCreatedSpace)
        {
            targetSpaceId = createdSpace.Id;
        }
        else
        {
            if (string.IsNullOrEmpty(spaceId)) return;

            targetSpaceId = spaceId;
        }

        using FoundationSystems.SpaceResult spaceResult = await spaceSystem.GetSpace(targetSpaceId);
        var space = spaceResult.GetSpace();
        await EnterSpaceAsync(space);
    }

    /// <summary>
    /// Searches available spaces for the user using the Foundation API.
    /// </summary>
    /// <returns> Just the Task object to await. </returns>
    private async Task SearchSpacesAsync()
    {
        using FoundationSystems.SpacesResult response = await spaceSystem.GetSpaces();
        FoundationCommon.Array<FoundationSystems.Space> spaces = response.GetSpaces();
        FoundationSystems.Space[] unitySpaces = ToUnityArray(spaces);

        Debug.Log("Got " + unitySpaces.Length + " Spaces");
    }
    
    /// <summary>
    /// Searches available spaces for the user using a GraphQL query.
    /// </summary>
    /// <returns> Just the Task object to await. </returns>
    private async Task SearchSpacesUsingGraphQLAsync()
    {
        var query = FormulateGraphQLQueryString();
        using FoundationSystems.GraphQLResult response = await graphQLSystem.RunQuery(query);
        var result = response.GetResponse();

        if (string.IsNullOrWhiteSpace(result))
        {
            Debug.Log("Got 0 Results with GraphQL. If this is unexpected, check your query and try again.");
            return;
        }

        GraphQLResponse myData = JsonUtility.FromJson<GraphQLResponse>(result);
        Debug.Log("Got " + myData.data.spaces.items.Length + " Spaces with GraphQL.");
    }

    /// <summary>
    /// Enters the specified space.
    /// </summary>
    /// <param name="space">Space to enter.</param>
    /// <returns> Just the Task object to await. </returns>
    private async Task EnterSpaceAsync(FoundationSystems.Space space)
    {
        await Task.Delay(100);
        using FoundationSystems.EnterSpaceResult enterResult = await spaceSystem.EnterSpace(space.Id, false);
        Debug.Log($"Joined Space {space.Name}");
        enteredSpace = true;
        await InitializeConnection(space.Id);
        await connection.SetAllowSelfMessagingFlag(true);
        StartTickLoop();
        Debug.Log("Connected to Multiplayer");

        entitySystem = connection.GetSpaceEntitySystem();
        entitySystem.OnEntityCreated += OnEntityCreated;

        var entity = await CreateAvatar();
        MoveAvatar(entity);
        await CreateAndUploadAssetAsync(space.Id);
    }

    private void EntityUpdate(object sender,
        (SpaceEntity entity, SpaceEntityUpdateFlags updateFLags, FoundationCommon.Array<ComponentUpdateInfo> info) e)
    {
        FoundationCommon.Vector3 receivedPosition = e.entity.GetPosition();
        var unityPosition = new Vector3(receivedPosition.X, receivedPosition.Y, receivedPosition.Z);

        Debug.Log($"Received Update for Entity {e.entity.GetName()} with position: {unityPosition}");
    }

    /// <summary>
    /// Requests the Foundation layer to exit the current space by setting the scope for the user's multiplayer service connection.
    /// </summary>
    private async void ExitSpaceAsync()
    {
        StopTickLoop();
        entitySystem.OnEntityCreated -= OnEntityCreated;

        await connection.Disconnect();
        connection.Dispose();
        Debug.Log("Disconnected from Multiplayer");
        await Task.Delay(100);

        spaceSystem.ExitSpace();
        Debug.Log("Exited Space");

        await Task.Delay(100);

        enteredSpace = false;
    }

    /// <summary>
    /// Requests the Foundation layer to delete the space that was previously created.
    /// </summary>
    private async void DeleteSpaceAsync()
    {
        if (createdSpace == null)
            return;
        Debug.Log($"Deleting Space {createdSpace.Name}");
        await spaceSystem.DeleteSpace(createdSpace.Id);
        Debug.Log($"Deleted Space {createdSpace.Name}");
    }

    /// <summary>
    /// Initializes a connection to multiplayer.
    /// </summary>
    /// <param name="spaceId">The id of the space which is entered.</param>
    /// <returns> Just the Task object to await. </returns>
    private async Task InitializeConnection(string spaceId)
    {
        connection = new MultiplayerConnection(spaceId);
        await connection.Connect();
        await connection.InitialiseConnection();
    }

    #endregion

    // Creating an avatar, moving an avatar

    #region Avatar

    /// <summary>
    /// Creates an Avatar for the user after joining a space.
    /// The avatar is used to represent the user in that space.
    /// </summary>
    /// <returns>SpaceEntity representing the avatar.</returns>
    private async Task<SpaceEntity> CreateAvatar()
    {
        SpaceEntity entity = await entitySystem.CreateAvatar("TestAvatar",
            new SpaceTransform(FoundationCommon.Vector3.Zero(), FoundationCommon.Vector4.Identity(),
                FoundationCommon.Vector3.One()), AvatarState.Idle, "id", AvatarPlayMode.Default);
        Debug.Log("Created Avatar.");
        return entity;
    }

    /// <summary>
    /// Moves the user's avatar and sends an update.
    /// </summary>
    /// <param name="spaceEntity">The Space entity which will be moved.</param>
    private void MoveAvatar(SpaceEntity spaceEntity)
    {
        var newPos = FoundationCommon.Vector3.One();
        var unityVector3 = new Vector3(newPos.X, newPos.Y, newPos.Z);
        spaceEntity.SetPosition(newPos);
        spaceEntity.QueueUpdate();

        Debug.Log($"Set Avatar Position to {unityVector3} and Queued Update.");
    }

    private void OnEntityCreated(object sender, SpaceEntity e)
    {
        e.OnUpdate += EntityUpdate;
    }

    #endregion

    // Assets - how to upload, retrieve, delete and search assets in space.

    #region Assets

    /// <summary>
    /// Demonstrates Creating, uploading, and deleting an asset to the Foundation layer.
    /// Assets are contained in Asset collections.
    /// </summary>
    /// <param name="spaceId">The space in which the asset collection will be uploaded.</param>
    private async Task CreateAndUploadAssetAsync(string spaceId)
    {
        //create an asset collection associated with our space
        string assetCollectionName = Guid.NewGuid().ToString();
        using FoundationSystems.AssetCollectionResult assetCollection = await assetSystem.CreateAssetCollection(spaceId,
            null, assetCollectionName,
            null, FoundationSystems.EAssetCollectionType.DEFAULT, null);

        // get the asset collection from the collection result
        var collection = assetCollection.GetAssetCollection();

        // create an asset
        using FoundationSystems.AssetResult assetResult = await assetSystem.CreateAsset(collection, exampleAssetName,
            null, null, FoundationSystems.EAssetType.IMAGE);

        // get the associated detail from the asset result
        var asset = assetResult.GetAsset();

        // set the asset filename
        asset.FileName = exampleAssetName;

        //read the file into buffer
        var bytes = File.ReadAllBytes(exampleAssetPath + exampleAssetName);
        int size = bytes.Length;
        var uploadFileDataPtr = Marshal.AllocHGlobal(size);
        Marshal.Copy(bytes, 0, uploadFileDataPtr, size);
        var source = new FoundationSystems.BufferAssetDataSource();
        source.Buffer = uploadFileDataPtr;
        source.BufferLength = (ulong)size;
        // set the mime type
        source.SetMimeType("image/png");

        // upload the file to the asset collection
        using FoundationSystems.UriResult uploadResult = await assetSystem.UploadAssetData(collection, asset, source);

        Debug.Log("Upload completed");
        // retrieve asset 
        using FoundationSystems.AssetCollectionsResult assetCollectionResult =
            await assetSystem.GetAssetCollectionsByCriteria(spaceId, null,
                FoundationSystems.EAssetCollectionType.DEFAULT, null, null, null, null);

        var assets = assetCollectionResult.GetAssetCollections();
        Debug.Log("Asset collection size:" + assets.Size());
        //delete asset
        await assetSystem.DeleteAsset(collection, asset);
        Debug.Log("Delete completed");
    }

    #endregion

    private void OnDestroy()
    {
        accountUI.OnSignIn -= SignInAsync;
        accountUI.OnSignUp -= SignUpAsync;
        accountUI.OnEnterSpace -= EnterSpaceAsync;
        accountUI.OnCreateSpace -= CreateSpaceAsync;
        accountUI.OnExitSpace -= ExitSpaceAsync;
        accountUI.OnDeleteSpace -= DeleteSpaceAsync;
    }

    #region Utils

    private T[] ToUnityArray<T>(FoundationCommon.Array<T> array)
    {
        if (array != null
            && array.PointerIsValid)
        {
            var unityArray = new T[array.Size()];

            for (int i = 0; i < unityArray.Length; i++)
            {
                unityArray[i] = array[(ulong)i];
            }

            return unityArray;
        }

        return null;
    }

    private FoundationCommon.Map<T, U> ToFoundationMap<T, U>(Dictionary<T, U> dict)
    {
        if (dict != null)
        {
            var map = new FoundationCommon.Map<T, U>();

            foreach (var keyVal in dict)
            {
                map[keyVal.Key] = keyVal.Value;
            }

            return map;
        }

        return null;
    }

    /// <summary>
    /// Creates abasic GraphQL query for getting 10 spaces from the Foundation layer.
    /// More info at https://graphql.org/learn/queries/
    /// </summary>
    /// <returns>The formulated string query.</returns>
    private string FormulateGraphQLQueryString()
    {
        return @"spaces(
        pagination: {limit: 10, skip: 0},
        filters:{})
			{
				itemTotalCount
				items
                {
                    groupId
                    name
                    description
                }
            }";
    }

    #endregion
}

[Serializable]
public class GraphQLResponse
{
    public Data data;
    public object extensions;
}

[Serializable]
public class Data
{
    public Spaces spaces;
}

[Serializable]
public class Spaces
{
    public int itemTotalCount;
    public SpaceResult[] items;
}

[Serializable]
public class SpaceResult
{
    public string groupId;
    public string name;
    public string description;
}