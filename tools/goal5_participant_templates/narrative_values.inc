namespace epidemic::gameplay::savegame::detail
{
template <> struct SnapshotValueCodec<narrative::NarrativeValue>
{
    template <typename IO> static void Transfer(IO& io, narrative::NarrativeValue& value)
    {
        std::uint8_t alternative = static_cast<std::uint8_t>(value.index());
        TransferSnapshot(io, alternative);
        if (alternative >= 5) throw std::runtime_error("invalid NarrativeValue alternative");
        if constexpr (IO::reading)
        {
            switch (alternative)
            {
            case 0: value.emplace<0>(); break;
            case 1: value.emplace<1>(); break;
            case 2: value.emplace<2>(); break;
            case 3: value.emplace<3>(); break;
            case 4: value.emplace<4>(); break;
            }
        }
        std::visit([&](auto& field) { TransferSnapshot(io, field); }, value);
    }
};
}
