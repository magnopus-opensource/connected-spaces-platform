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

    public class NativeClassWrapper
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
    }
}
