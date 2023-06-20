
namespace Tests
{
    public class Result<T>
    {
        public T Value;

        public static implicit operator T(Result<T> result) => result.Value;
    }
}
