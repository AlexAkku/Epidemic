#include <Epidemic/GameFramework/Presentation/presentation.h>
#include <Epidemic/GameFramework/Session/profile_store.h>
#include <Epidemic/build_info.h>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argc, char **argv) {
    try {
        using namespace epidemic::gameplay;
        std::cout << "Epidemic " << epidemic::build::Version << " ABI " << epidemic::build::AbiVersion << '\n';
        if (argc > 1 && std::string_view(argv[1]) == "--headless") {
            return 0;
        }
        if (argc > 2 && std::string_view(argv[1]) == "--profile") {
            session::SaveProfilePayload(argv[2], "minimal-client");
            if (session::LoadProfilePayload(argv[2]) != "minimal-client") {
                throw std::runtime_error("Profile round trip failed");
            }
            return 0;
        }
        presentation::Host host("Epidemic minimal game");
        presentation::Canvas canvas(host, {}, 0);
        if (argc > 1 && std::string_view(argv[1]) == "--tool") {
            host.ConfigureStartup(false);
        }
        host.Run([](const auto &) {}, [&] {
            canvas.Begin();
            const auto extent = canvas.LogicalExtent();
            canvas.Rect(0, 0, extent.x, extent.y, 0x123344);
            canvas.Text(L"Epidemic SDK client", 120, 200, 40, 0xffffff);
            canvas.End();
            host.RequestStop();
        });
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
