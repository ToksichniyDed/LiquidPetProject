//
// Created by DED on 10.09.2026.
//

#ifndef LIQUIDPETPROJECT_IEVENTHANDLER_H
#define LIQUIDPETPROJECT_IEVENTHANDLER_H

namespace shared::messaging {

    class IEventHandler
    {
    public:
        virtual ~IEventHandler() = default;

        // Возвращает true, если сообщение обработано успешно (оффсет можно коммитить),
        // false — обработка не удалась, оффсет НЕ коммитится, сообщение будет
        // повторно доставлено при следующем poll/рестарте.
        [[nodiscard]] virtual bool handle(const std::string& payload) = 0;
    };

}  // namespace shared::messaging

#endif //LIQUIDPETPROJECT_IEVENTHANDLER_H
