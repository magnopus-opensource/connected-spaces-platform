using System;
using System.Runtime.InteropServices;


namespace Csp
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct NativePointer
    {
        public IntPtr Pointer;
        readonly byte _ownsOwnData;

        public bool OwnsOwnData => _ownsOwnData != 0;

        internal NativePointer(IntPtr pointer, byte ownsOwnData)
        {
            Pointer = pointer;
            _ownsOwnData = ownsOwnData;
        }

        public static NativePointer Zero => new NativePointer(IntPtr.Zero, 0);
    }

    public class NativeClassWrapper : IDisposable
    {
        internal IntPtr _ptr;
        internal bool _ownsPtr;
        internal bool _disposed = false;

        internal virtual string _safeTypeName { get; }

        public bool PointerIsValid => _ptr != IntPtr.Zero;

        public NativeClassWrapper() { }

        internal NativeClassWrapper(NativePointer ptr)
        {
            _ptr = ptr.Pointer;
            _ownsPtr = ptr.OwnsOwnData;
        }

        protected virtual void DisposeNativeObject() { }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                try
                {
                    if (_ownsPtr)
                    {
                        if (!disposing)
                        {
                            //TODO: Log that the object wasn't disposed and is being destroyed by finalizer
                        }

                        if (_ptr == IntPtr.Zero)
                        {
                            throw new InvalidOperationException("Internal pointer was null");
                        }

                        if (CSPFoundation.GetIsInitialised())
                        {
                            DisposeNativeObject();
                        }
                    }
                }
                finally
                {
                    _ptr = IntPtr.Zero;
                    _disposed = true;
                }
            }
        }

        ~NativeClassWrapper()
        {
             // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
             Dispose(false);
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
}
