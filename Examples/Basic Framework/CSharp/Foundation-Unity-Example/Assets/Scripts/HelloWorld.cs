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
using CspCommon = Csp.Common;
using CspSystems = Csp.Systems;
using CspAsset = Csp.Systems.Asset;
using UnityEngine;
using UnityEngine.Windows;

public class HelloWorld : MonoBehaviour
{
    [SerializeField] private AccountUI accountUI;

    private const string endPointUri = "https://ogs-ostage.magnoboard.com";
    private const string TenantKey = "CSP_HELLO_WORLD";
    private const string defaultSpaceSite = "Void";
    private const int TickDelayMilliseconds = 1000 / 60; //60fps
    private readonly string exampleAssetName = "example.png";
    private readonly string exampleAssetPath = "Assets/StreamingAssets/";
    private bool cspHasStarted;
    private bool enteredSpace;
    private CspSystems.UserSystem userSystem;
    private CspSystems.SpaceSystem spaceSystem;
    private CspSystems.AssetSystem assetSystem;
    private CspSystems.GraphQLSystem graphQLSystem;
    private SpaceEntitySystem entitySystem;
    private CancellationTokenSource cancellationTokenSource;
    private CspSystems.Space createdSpace;
    private string userId;

    // Initialisation of Csp

    #region Initialisation

    private void Awake()
    {
        var userAgent = new ClientUserAgent()
        {
            CHSEnvironment = "OStage",
            ClientEnvironment = "Stage",
            ClientOS = SystemInfo.operatingSystem,
            ClientSKU = "csp-cSharp-examples",
            ClientVersion = "0.0.4",
            CSPVersion = CSPFoundation.GetBuildID()
        };

        Application.quitting += QuitFoundation;

        accountUI.OnSignIn += SignInAsync;
        accountUI.OnSignUp += SignUpAsync;
        accountUI.OnEnterSpace += EnterSpaceAsync;
        accountUI.OnCreateSpace += CreateSpaceAsync;
        accountUI.OnExitSpace += ExitSpaceAsync;
        accountUI.OnDeleteSpace += DeleteSpaceAsync;

        StartCspFoundation(endPointUri, TenantKey, userAgent);
    }

    /// <summary>
    /// Starts the underlying Foundation systems, you should also
    /// call <see cref="StopFoundation"/> during the consuming application's shutdown process.
    /// </summary>
    /// <param name="backendEndpoint">The endpoint url for the backend services.</param>
    /// <param name="tenant">The assigned Tenant value for this application.</param>
    /// <param name="userAgent">Identifiable information to this application client.</param>
    private void StartCspFoundation(string backendEndpoint, string tenant, ClientUserAgent userAgent)
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

        userAgent.Dispose();
        cspHasStarted = true;
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
        userSystem = CspSystems.SystemsManager.Get().GetUserSystem();
        spaceSystem = CspSystems.SystemsManager.Get().GetSpaceSystem();
        assetSystem = CspSystems.SystemsManager.Get().GetAssetSystem();
        graphQLSystem = CspSystems.SystemsManager.Get().GetGraphQLSystem();
        entitySystem = CspSystems.SystemsManager.Get().GetSpaceEntitySystem();
    }

    /// <summary>
    /// Shuts down the underlying Foundation systems, this should be called by the consuming application
    /// during application shutdown once all of the dependant systems have been shutdown.
    /// </summary>
    private void StopFoundation()
    {
        Debug.Log("Shutting down Csp Foundation ...");
        bool successShutdown = CSPFoundation.Shutdown();
        if (!successShutdown)
        {
            Debug.Log("Failed to shut down Foundation. Error is within Foundation package.");
            return;
        }

        Debug.Log("Shutdown Csp Foundation");
        cspHasStarted = false;
    }

    private void QuitFoundation()
    {
        Application.quitting -= QuitFoundation;

        if (cspHasStarted)
        {
            if (enteredSpace)
            {
                ExitSpaceAsync();
            }

            StopTickLoop();

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
        if(await LoginAsync(email, password))
        {
            StartTickLoop();

            await SearchSpacesAsync();
            await SearchSpacesUsingGraphQLAsync();
        }
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

        using CspSystems.ProfileResult result =
            await userSystem.CreateUser(null, null, email, password, false, true, null,null);
        using CspSystems.Profile profile = result.GetProfile();

        Debug.Log($"Created account with Email {profile.Email}. Please check your email to verify your account.");
    }

    /// <summary>
    /// Requests the Foundation layer to Login with email and password.
    /// </summary>
    /// <param name="email"> Email of the user. </param>
    /// <param name="password"> Password of the user. </param>
    /// <returns> Just the Task object to await.</returns>
    private async Task<bool> LoginAsync(string email, string password)
    {
        Debug.Log($"Logging in with Email {email} ...");

        using CspSystems.LoginStateResult loginResult = await userSystem.Login(string.Empty, email, password, true);
        if (loginResult.GetResultCode() == Csp.Systems.EResultCode.Success)
        {
            // Cache user ID for later use.
            using var loginState = loginResult.GetLoginState();
            Debug.Log($"Login state: {loginState.State}.");
            
            // Cache user ID for later use.
            userId = loginState.UserId;
            Debug.Log($"Logged in with Email {email}.");
            var connectionState = CspSystems.SystemsManager.Get().GetMultiplayerConnection().GetConnectionState();
            Debug.Log($"Multiplayer Connection state: {connectionState} ");
            return true;
        }
        else
        {
            Debug.LogError($"Failed to log in. Reason: {loginResult.GetResponseBody()}");
            return false;
        }
    }

    /// <summary>
    /// Requests the Foundation layer to make the user logout.
    /// Not used for this example as shutting down the Foundation layer will automatically log the user out.
    /// </summary>
    /// <returns> Just the Task object to await.</returns>
    private async Task LogoutAsync()
    {
        StopTickLoop();
        
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
        using CspSystems.SpaceResult spaceResult = await spaceSystem.CreateSpace(spaceName, string.Empty,
            CspSystems.SpaceAttributes.Private, null, ToFoundationMap(metadata), null);

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

        using CspSystems.SpaceResult spaceResult = await spaceSystem.GetSpace(targetSpaceId);
        using var space = spaceResult.GetSpace();
        await EnterSpaceAsync(space);
    }

    /// <summary>
    /// Searches available spaces for the user using the Foundation API.
    /// </summary>
    /// <returns> Just the Task object to await. </returns>
    private async Task SearchSpacesAsync()
    {
        using CspSystems.SpacesResult response = await spaceSystem.GetSpaces();
        CspCommon.Array<CspSystems.Space> spaces = response.GetSpaces();
        CspSystems.Space[] unitySpaces = ToUnityArray(spaces);

        Debug.Log("Got " + unitySpaces.Length + " Spaces");
    }
    
    /// <summary>
    /// Searches available spaces for the user using a GraphQL query.
    /// </summary>
    /// <returns> Just the Task object to await. </returns>
    private async Task SearchSpacesUsingGraphQLAsync()
    {
        var query = FormulateGraphQLQueryString();
        using CspSystems.GraphQLResult response = await graphQLSystem.RunQuery(query);
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
    private async Task EnterSpaceAsync(CspSystems.Space space)
    {
        await Task.Delay(100);
        
        entitySystem.OnEntityCreated += OnEntityCreated;
        
        using CspSystems.NullResult enterResult = await spaceSystem.EnterSpace(space.Id);
        if (enterResult.GetResultCode() != CspSystems.EResultCode.Success)
        {
            Debug.LogError($"Failed to enter space. Result code: {enterResult.GetResultCode()}. Aborting...");
            entitySystem.OnEntityCreated -= OnEntityCreated;
            return;
        }
        
        Debug.Log($"Entered Space {space.Name}");
        enteredSpace = true;
        var entity = await CreateAvatar();
        MoveAvatar(entity);
        await CreateAndUploadAssetAsync(space.Id);
    }

    private void EntityUpdate(object sender,
        (SpaceEntity entity, SpaceEntityUpdateFlags updateFLags, CspCommon.Array<ComponentUpdateInfo> info) e)
    {
        CspCommon.Vector3 receivedPosition = e.entity.GetPosition();
        var unityPosition = new Vector3(receivedPosition.X, receivedPosition.Y, receivedPosition.Z);

        Debug.Log($"Received Update for Entity {e.entity.GetName()} with position: {unityPosition}");
    }

    /// <summary>
    /// Requests the Foundation layer to exit the current space by setting the scope for the user's multiplayer service connection.
    /// </summary>
    private async void ExitSpaceAsync()
    {
        entitySystem.OnEntityCreated -= OnEntityCreated;

        await Task.Delay(100);

        await spaceSystem.ExitSpace();
        Debug.Log("Exited Space.");

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
            new SpaceTransform(CspCommon.Vector3.Zero(), CspCommon.Vector4.Identity(),
                CspCommon.Vector3.One()), AvatarState.Idle, userId, AvatarPlayMode.Default);
        Debug.Log("Created Avatar.");
        return entity;
    }

    /// <summary>
    /// Moves the user's avatar and sends an update.
    /// </summary>
    /// <param name="spaceEntity">The Space entity which will be moved.</param>
    private void MoveAvatar(SpaceEntity spaceEntity)
    {
        var newPos = CspCommon.Vector3.One();
        var unityVector3 = new Vector3(newPos.X, newPos.Y, newPos.Z);
        spaceEntity.SetPosition(newPos);
        spaceEntity.QueueUpdate();

        Debug.Log($"Set Avatar Position to {unityVector3} and Queued Update.");
        spaceEntity.Dispose();
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
        using CspSystems.AssetCollectionResult assetCollection = await assetSystem.CreateAssetCollection(spaceId,
            null, assetCollectionName,
            null, CspSystems.EAssetCollectionType.DEFAULT, null);

        // get the asset collection from the collection result
        var collection = assetCollection.GetAssetCollection();

        // create an asset
        using CspSystems.AssetResult assetResult = await assetSystem.CreateAsset(collection, exampleAssetName,
            null, null, CspSystems.EAssetType.IMAGE);

        // get the associated detail from the asset result
        var asset = assetResult.GetAsset();

        // set the asset filename
        asset.FileName = exampleAssetName;

        //read the file into buffer
        var bytes = File.ReadAllBytes(exampleAssetPath + exampleAssetName);
        int size = bytes.Length;
        var uploadFileDataPtr = Marshal.AllocHGlobal(size);
        Marshal.Copy(bytes, 0, uploadFileDataPtr, size);
        var source = new CspSystems.BufferAssetDataSource();
        source.Buffer = uploadFileDataPtr;
        source.BufferLength = (ulong)size;
        // set the mime type
        source.SetMimeType("image/png");

        // upload the file to the asset collection
        using CspSystems.UriResult uploadResult = await assetSystem.UploadAssetData(collection, asset, source);

        Debug.Log("Upload completed");
        // retrieve asset 
        
        using var cspArraySpaces = ToCspArray(new[] { spaceId });
        using CspSystems.AssetCollectionsResult assetCollectionResult =
            await assetSystem.FindAssetCollections(null, null,
                 null, null, null, cspArraySpaces, 0,0);

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
        
        if (createdSpace != null)
        {
            createdSpace.Dispose();
        }
    }

    #region Utils

    private CspCommon.Array<T> ToCspArray<T>(T[] array)
    {
        if (array != null)
        {
            ulong size = (ulong)array.Length;
            var cspArray = new CspCommon.Array<T>(size);

            for (ulong i = 0; i < size; i++)
            {
                cspArray[i] = array[i];
            }

            return cspArray;
        }

        return null;
    }
    
    private T[] ToUnityArray<T>(CspCommon.Array<T> array)
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

    private CspCommon.Map<T, U> ToFoundationMap<T, U>(Dictionary<T, U> dict)
    {
        if (dict != null)
        {
            var map = new CspCommon.Map<T, U>();

            foreach (var keyVal in dict)
            {
                map[keyVal.Key] = keyVal.Value;
            }

            return map;
        }

        return null;
    }

    /// <summary>
    /// Creates abasic GraphQL query for getting 10 spaces from the Csp layer.
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