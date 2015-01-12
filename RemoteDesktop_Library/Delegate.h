#ifndef DELGATE123_H
#define DELGATE123_H

namespace RemoteDesktop{
	template<typename return_type, typename... params>
	class Delegate
	{
		typedef return_type(*Type)(void* callee, params...);
	public:
		Delegate() : fpCallee(nullptr)
			, fpCallbackFunction(nullptr){}

		Delegate(void* callee, Type function)
			: fpCallee(callee)
			, fpCallbackFunction(function) {}

		template <class T, return_type(T::*TMethod)(params...)>
		static Delegate from_function(T* callee)
		{
			Delegate d(callee, &methodCaller<T, TMethod>);
			return d;
		}
		operator bool() const{
			return fpCallee != nullptr && fpCallbackFunction != nullptr;
		}
		return_type operator()(params... xs) const
		{
			return (*fpCallbackFunction)(fpCallee, xs...);
		}

	private:

		void* fpCallee;
		Type fpCallbackFunction;

		template <class T, return_type(T::*TMethod)(params...)>
		static return_type methodCaller(void* callee, params... xs)
		{
			T* p = static_cast<T*>(callee);
			return (p->*TMethod)(xs...);
		}
	};
	template<typename T, typename return_type, typename... params>
	struct DelegateMaker
	{
		template<return_type(T::*foo)(params...)>
		static return_type methodCaller(void* o, params... xs)
		{
			return (static_cast<T*>(o)->*foo)(xs...);
		}

		template<return_type(T::*foo)(params...)>
		inline static Delegate<return_type, params...> Bind(T* o)
		{
			return Delegate < return_type,
				params... > (o, &DelegateMaker::methodCaller<foo>);
		}
	};

	template<typename T, typename return_type, typename... params>
	DelegateMaker<T, return_type, params... >
		makeDelegate(return_type(T::*)(params...))
	{
		return DelegateMaker<T, return_type, params...>();
	}

#define DELEGATE(foo) (makeDelegate(foo).Bind<foo>(this))
}

#endif