#pragma once

#include <experimental/coroutine>
#include <utility>
#include <vector>

#include <Windows.h>

namespace Chars {

struct Coro
{
    struct promise_type
    {
        std::pair<WPARAM, LPARAM> event{};
        LRESULT                   result{};
        std::vector<wchar_t>      buffer;

        constexpr auto await_transform(HWND) noexcept
        {
            struct Awaiter
            {
                constexpr bool await_ready() const noexcept
                {
                    return false;
                }

                void await_suspend(handle_type) noexcept
                {
                }

                constexpr auto await_resume() noexcept
                {
                    return promise;
                }

                promise_type* promise;
            };

            return Awaiter{this};
        }

        constexpr auto initial_suspend() const noexcept
        {
            return std::experimental::suspend_never{};
        }

        constexpr auto final_suspend() const noexcept
        {
            return std::experimental::suspend_always{};
        }

        auto get_return_object() noexcept
        {
            return Coro{handle_type::from_promise(*this)};
        }

        constexpr auto return_void() const noexcept
        {
            return std::experimental::suspend_never{};
        }

        constexpr void yield_value(LRESULT ret) noexcept
        {
            result = ret;
        }
    };

    using handle_type = std::experimental::coroutine_handle<promise_type>;

    LRESULT OnChar(WPARAM wParam, LPARAM lParam) noexcept
    {
        handle.promise().event = {wParam, lParam};

        handle.resume();

        return handle.promise().result;
    }

    LPCWSTR Text() const noexcept
    {
        return handle.promise().buffer.data();
    }

    int TextSize() const noexcept
    {
        return static_cast<int>(handle.promise().buffer.size());
    }

    Coro() noexcept = default;

    explicit operator bool() const noexcept
    {
        return !!handle;
    }

private:
    Coro(handle_type handle) noexcept : handle(handle)
    {
    }

    handle_type handle{};
}; // namespace Chars

Coro OnChar(HWND hWnd)
{
    for (auto promise = co_await hWnd;; promise = co_await hWnd)
    {
        switch (auto theChar = static_cast<wchar_t>(promise->event.first);
                theChar)
        {
        case L'\b':
            if (!promise->buffer.empty())
            {
                promise->buffer.pop_back();
            }
            break;

        default:
            promise->buffer.push_back(theChar);
            break;
        }

        InvalidateRect(hWnd, nullptr, TRUE);

        promise->result = 0;
    }
}

} // namespace Chars
