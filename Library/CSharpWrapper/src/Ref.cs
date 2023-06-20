using System;


namespace Csp
{
    /// <summary>
    /// A type-safe wrapper around a pointer to an unmanaged type
    /// </summary>
    /// <typeparam name="T">The type of the object the pointer refers to</typeparam>
    public struct Ref<T> where T : unmanaged
    {
        internal IntPtr Pointer;

        internal Ref(IntPtr Pointer)
        {
            this.Pointer = Pointer;
        }

        public static implicit operator T(Ref<T> value) => value.Get();

        public unsafe T Get() =>*(T*)Pointer.ToPointer();
        
        public void Set(T value)
        {
            unsafe
            {
                *(T*)Pointer.ToPointer() = value;
            }
        }

        public bool IsValid() => Pointer != IntPtr.Zero;
    }
}
