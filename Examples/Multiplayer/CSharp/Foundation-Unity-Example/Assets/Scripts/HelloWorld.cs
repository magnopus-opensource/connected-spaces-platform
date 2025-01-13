// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Csp;
using Csp.Multiplayer;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using CspCommon = Csp.Common;
using CspSystems = Csp.Systems;
using UnityEngine;

public class HelloWorld : MonoBehaviour
{
    [SerializeField] private AccountUI accountUI;
    [SerializeField] private InSpaceUI inSpaceUI;
    [SerializeField] private LocalPlayer localPlayerPrefab;
    [SerializeField] private RemotePlayer remotePlayerPrefab;

    private const string endPointUri = "https://ogs.magnopus-stg.cloud";
    private const string TenantKey = "CSP_HELLO_WORLD";
    private const string defaultSpaceSite = "Void";
    private const int TickDelayMilliseconds = 1000 / 60; //60fps
    private bool cspHasStarted;
    private bool enteredSpace;
    private CspSystems.UserSystem userSystem;
    private CspSystems.SpaceSystem spaceSystem;
    private CspSystems.GraphQLSystem graphQLSystem;
    private SpaceEntitySystem entitySystem;
    private CancellationTokenSource cancellationTokenSource;
    private CspSystems.Space createdSpace;

    private string userId;
    private LocalPlayer localPlayer;
    private List<RemotePlayer> remotePlayers = new List<RemotePlayer>();
    private List<SpaceEntity> pendingRemoteEntities = new List<SpaceEntity>();

    #region Unity Callbacks

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

        Application.quitting += QuitCSPFoundation;

        accountUI.OnSignIn += SignInAsync;
        accountUI.OnSignUp += SignUpAsync;
        accountUI.OnEnterSpace += EnterSpaceAsync;
        accountUI.OnCreateSpace += CreateSpaceAsync;
        accountUI.OnDeleteSpace += DeleteSpaceAsync;

        inSpaceUI.OnExitSpace += ExitSpaceAsync;

        StartCSPFoundation(endPointUri, TenantKey, userAgent);
    }

    private void Update()
    {
        ProcessPendingRemoteEntities();
    }

    private void OnDestroy()
    {
        accountUI.OnSignIn -= SignInAsync;
        accountUI.OnSignUp -= SignUpAsync;
        accountUI.OnEnterSpace -= EnterSpaceAsync;
        accountUI.OnCreateSpace -= CreateSpaceAsync;
        accountUI.OnDeleteSpace -= DeleteSpaceAsync;

        if (createdSpace != null)
        {
            createdSpace.Dispose();
        }
    }

    #endregion

    // Initialisation of Csp

    #region Initialisation

    /// <summary>
    /// Starts the underlying CSP Foundation systems, you should also
    /// call <see cref="StopCSPFoundation"/> during the consuming application's shutdown process.
    /// </summary>
    /// <param name="backendEndpoint">The endpoint url for the backend services.</param>
    /// <param name="tenant">The assigned Tenant value for this application.</param>
    /// <param name="userAgent">Identifiable information to this application client.</param>
    private void StartCSPFoundation(string backendEndpoint, string tenant, ClientUserAgent userAgent)
    {
        Debug.Log("Initializing CSP Foundation ...");
        bool successInit = CSPFoundation.Initialise(backendEndpoint, tenant);
        if (!successInit)
        {
            Debug.Log("Failed to initialize CSP Foundation. Error is within CSP package.");
            return;
        }

        CSPFoundation.SetClientUserAgentInfo(userAgent);
        Debug.Log("Initialized CSP Foundation.");

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
        graphQLSystem = CspSystems.SystemsManager.Get().GetGraphQLSystem();
        entitySystem = CspSystems.SystemsManager.Get().GetSpaceEntitySystem();
    }

    /// <summary>
    /// Shuts down the underlying CSP Foundation systems, this should be called by the consuming application
    /// during application shutdown once all of the dependant systems have been shutdown.
    /// </summary>
    private void StopCSPFoundation()
    {
        Debug.Log("Shutting down CSP Foundation ...");
        bool successShutdown = CSPFoundation.Shutdown();
        if (!successShutdown)
        {
            Debug.Log("Failed to shut down CSP Foundation. Error is within CSP Foundation package.");
            return;
        }

        Debug.Log("Shutdown CSP Foundation");
        cspHasStarted = false;
    }

    private void QuitCSPFoundation()
    {
        Application.quitting -= QuitCSPFoundation;

        if (cspHasStarted)
        {
            if (enteredSpace)
            {
                ExitSpaceAsync();
            }

            StopTickLoop();

            StopCSPFoundation();
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
        if (await LoginAsync(email, password))
        {
            StartTickLoop();

            await SearchSpacesAsync();
            await SearchSpacesUsingGraphQLAsync();
        }
    }

    /// <summary>
    /// Requests the CSP layer to create a new account for the user using the given email and password.
    /// </summary>
    /// <param name="email"> email of the new account.</param>
    /// <param name="password"> password of the new account.</param>
    /// <returns> Just the Task object to await.</returns>
    private async Task CreateUserAsync(string email, string password)
    {
        Debug.Log($"Creating account with Email {email} ...");

        using CspSystems.ProfileResult result =
            await userSystem.CreateUser(null, null, email, password, false, true, null, null);
        using CspSystems.Profile profile = result.GetProfile();

        Debug.Log($"Created account with Email {profile.Email}. Please check your email to verify your account.");
    }

    /// <summary>
    /// Requests the CSP layer to Login with email and password.
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
    /// Requests the CSP layer to make the user logout.
    /// Not used for this example as shutting down the CSP layer will automatically log the user out.
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

        // Create a public space so other users can join without invite.
        using CspSystems.SpaceResult spaceResult = await spaceSystem.CreateSpace(spaceName, string.Empty,
            CspSystems.SpaceAttributes.Public, null, ToCSPMap(metadata), null);

        if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
        {
            createdSpace = spaceResult.GetSpace();
            Debug.Log($"Created space with name: {createdSpace.Name} and ID: {createdSpace.Id}");
        }
        else
        {
            Debug.LogError($"Failed to create space.");
        }
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

        if (spaceResult.GetResultCode() == Csp.Systems.EResultCode.Success)
        {
            using var space = spaceResult.GetSpace();
            await EnterSpaceAsync(space);
        }
        else
        {
            Debug.LogError($"Failed to get space \"{targetSpaceId}\". Unable to enter.");
        }
    }

    /// <summary>
    /// Searches available spaces for the user using the CSP Foundation API.
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

        await SpawnLocalAvatar();

        // Disable account UI.
        accountUI.gameObject.SetActive(false);

        // Enable in-space UI.
        inSpaceUI.gameObject.SetActive(true);
    }

    /// <summary>
    /// Requests the CSP layer to exit the current space by setting the scope for the user's multiplayer service connection.
    /// </summary>
    private async void ExitSpaceAsync()
    {
        entitySystem.OnEntityCreated -= OnEntityCreated;

        await CleanupAvatars();

        await spaceSystem.ExitSpace();
        Debug.Log("Exited Space");
        await Task.Delay(100);

        enteredSpace = false;

        // Enable account UI.
        accountUI.gameObject.SetActive(true);

        // Disable in-space UI.
        inSpaceUI.gameObject.SetActive(false);
    }

    /// <summary>
    /// Requests the CSP layer to delete the space that was previously created.
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
    private async Task SpawnLocalAvatar()
    {
        using SpaceEntity entity = await entitySystem.CreateAvatar($"user-{userId}",
            new SpaceTransform(CspCommon.Vector3.Zero(), CspCommon.Vector4.Identity(),
                CspCommon.Vector3.One()), AvatarState.Idle, userId, AvatarPlayMode.Default);

        localPlayer = Instantiate(localPlayerPrefab, Vector3.zero, Quaternion.identity);
        localPlayer.Initialize(entity);

        Debug.Log("Created avatar for local player.");
    }

    private void OnEntityCreated(object sender, SpaceEntity spaceEntity)
    {
        Debug.Log("OnEntityCreated triggered");
        SpaceEntityType entityType = spaceEntity.GetEntityType();
        if (entityType == SpaceEntityType.Avatar)
        {
            Debug.Log($"Avatar entity created: {spaceEntity.GetName()}, {spaceEntity.GetId()}");

            // Cache the entity for spawning it later. This is because CSP fires EntityCreated on its own thread, 
            // we can't call GameObject.Instantiate() right away outside of the main thread.
            lock (pendingRemoteEntities)
            {
                pendingRemoteEntities.Add(spaceEntity);
            }
        }
    }

    private void ProcessPendingRemoteEntities()
    {
        lock (pendingRemoteEntities)
        {
            if (pendingRemoteEntities.Count > 0)
            {
                // Create avatar GameObject for all pending entities.
                foreach (var entity in pendingRemoteEntities)
                {
                    SpaceEntityType entityType = entity.GetEntityType();
                    if (entityType == SpaceEntityType.Avatar)
                    {
                        Debug.Log($"Spawn remote avatar: {entity.GetName()}, {entity.GetId()}");
                        CreateRemoteAvatarGameObject(entity);
                    }
                }

                pendingRemoteEntities.Clear();
            }
        }
    }

    /// <summary>
    /// Creates a remote avatar GameObject representing the given entity in scene. 
    /// </summary>
    /// <param name="spaceEntity"></param>
    private void CreateRemoteAvatarGameObject(SpaceEntity spaceEntity)
    {
        // Get entity transform values.
        using var cspVectorPosition = spaceEntity.GetPosition();
        using var cspQuatRotation = spaceEntity.GetRotation();
        using var cspVectorScale = spaceEntity.GetScale();
        
        Vector3 pos = cspVectorPosition.ToUnityVector().ToUnityPositionFromGLTF();
        Quaternion rot = cspQuatRotation.ToUnityQuaternion().ToUnityRotationFromGLTF();
        Vector3 scale = cspVectorScale.ToUnityVector().ToUnityScaleFromGLTF();

        // Create remote 
        RemotePlayer remotePlayer = Instantiate(remotePlayerPrefab, pos, rot);
        remotePlayer.transform.localScale = scale;
        remotePlayer.name = spaceEntity.GetName();

        remotePlayer.Initialize(spaceEntity);
        remotePlayers.Add(remotePlayer);
    }

    /// <summary>
    /// Destroys all remote player objects and despawns local player.
    /// </summary>
    /// <returns></returns>
    private async Task CleanupAvatars()
    {
        // Remove remote players from the scene
        foreach (RemotePlayer rPlayer in remotePlayers)
        {
            if (rPlayer != null)
            {
                Destroy(rPlayer.gameObject);
            }
        }
        remotePlayers.Clear();

        // Clear pending remote entity list.
        lock (pendingRemoteEntities)
        {
            pendingRemoteEntities.Clear();
        }

        // Remove local player for remote people
        if (localPlayer != null)
        {
            bool successfulDestroyPlayer = await entitySystem.DestroyEntity(localPlayer.Entity);
            if (successfulDestroyPlayer)
            {
                Debug.Log("Successfully removed the local player from the space!");
            }
            else
            {
                Debug.LogError("Failed to remove the local player from the space!");
            }

            // Remove local player from the scene
            Destroy(localPlayer.gameObject);
            localPlayer = null;
        }
    }

    #endregion

    #region Utils

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

    private CspCommon.Map<T, U> ToCSPMap<T, U>(Dictionary<T, U> dict)
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