// money_tables.hpp - ROB RUN (IL2CPP / Unity x64) table traversal module
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <cstring>
#include "../Misc/Vector.h"
#include "RobRunMemory.h"

namespace rob_run {

    enum class TableType {
        MoneyWorkbench,   // Money Workbench (Clean + Dirty cash stacks)
        CollectionTable   // Collection Table (Reward cash collections)
    };

    struct TableRecord {
        TableType type{ TableType::MoneyWorkbench };
        uintptr_t address{ 0 };
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        float yaw{ 0.0f };
        int32_t dirty_cash{ 0 };
        int32_t clean_cash{ 0 };
        int32_t reward_cash{ 0 };
        int32_t total_cash{ 0 };
    };

    // Traversal offsets definition matching target managed runtime models
    namespace offsets {
        namespace battle {
            constexpr uint32_t world_entities_primary = 0x518;
            constexpr uint32_t world_entities_secondary = 0x60;
        }

        namespace world_entities {
            constexpr uint32_t money_workbenches_dict = 0x530;
            constexpr uint32_t collection_tables_dict = 0x528;
        }

        namespace money_workbench {
            constexpr uint32_t description_model = 0x20;
            constexpr uint32_t location_description = 0x10;
            constexpr uint32_t position = 0x20;
            constexpr uint32_t orientation = 0x2C;
            constexpr uint32_t yaw_offset = 0x30; // orientation.y

            constexpr uint32_t data_container = 0x10;
            constexpr uint32_t dirty_amount_ptr = 0x20;
            constexpr uint32_t clean_amount_ptr = 0x28;
            constexpr uint32_t cash_value = 0x18;
        }

        namespace collection_table {
            constexpr uint32_t description_model = 0x10;
            constexpr uint32_t location_description = 0x10;
            constexpr uint32_t position = 0x20;
            constexpr uint32_t orientation = 0x2C;
            constexpr uint32_t yaw_offset = 0x30; // orientation.y

            constexpr uint32_t reward_model = 0x30;
            constexpr uint32_t reward_data = 0x10;
            constexpr uint32_t cash_amount_primary = 0x18;
            constexpr uint32_t cash_amount_secondary = 0x20;
            constexpr uint32_t cash_value = 0x18;
        }

        namespace il2cpp_dictionary {
            constexpr uint32_t entries_ptr = 0x18;
            constexpr uint32_t count = 0x20;
            constexpr uint32_t array_bounds_length = 0x18;
            constexpr uint32_t array_data_start = 0x20;
        }
    }

    // Abstract reader interface enabling internal direct read or external ReadProcessMemory / DMA
    using ReadMemoryFn = std::function<bool(uintptr_t address, void* buffer, size_t size)>;

    class MoneyTableReader {
    public:
        explicit MoneyTableReader(ReadMemoryFn reader = nullptr)
            : read_memory_(std::move(reader)) {}

        void set_memory_reader(ReadMemoryFn reader) {
            read_memory_ = std::move(reader);
        }

        template <typename T>
        bool read(uintptr_t address, T& out_value) const {
            if (!address) {
                return false;
            }
            if (read_memory_) {
                return read_memory_(address, &out_value, sizeof(T));
            }
            return RobRunMemory::ReadBytes(address, &out_value, sizeof(T));
        }

        uintptr_t read_ptr(uintptr_t address) const {
            uintptr_t ptr = 0;
            if (read(address, ptr)) {
                return ptr;
            }
            return 0;
        }

        // Resolves WorldEntitiesModel pointer from BattleModel root
        uintptr_t resolve_world_entities(uintptr_t battle_model) const {
            if (!battle_model) {
                return 0;
            }
            uintptr_t world = read_ptr(battle_model + offsets::battle::world_entities_primary);
            if (!world) {
                world = read_ptr(battle_model + offsets::battle::world_entities_secondary);
            }
            return world;
        }

        // Traverses IL2CPP Dictionary<int, ModelPtr> entries
        std::vector<uintptr_t> extract_dictionary_values(uintptr_t dict_address) const {
            std::vector<uintptr_t> values;
            if (!dict_address) {
                return values;
            }

            // If using default memory reader, delegate to optimized RobRunMemory
            if (!read_memory_) {
                std::vector<RobRunMemory::DictEntry> entries;
                if (RobRunMemory::ReadDictInt(dict_address, entries, false)) {
                    for (const auto& e : entries) {
                        if (e.value > 0x10000) {
                            values.push_back(static_cast<uintptr_t>(e.value));
                        }
                    }
                    return values;
                }
            }

            int32_t count = 0;
            if (!read(dict_address + offsets::il2cpp_dictionary::count, count) || count <= 0 || count > 4096) {
                return values;
            }

            uintptr_t entries_array = read_ptr(dict_address + offsets::il2cpp_dictionary::entries_ptr);
            if (!entries_array) {
                return values;
            }

            int32_t array_len = 0;
            if (!read(entries_array + offsets::il2cpp_dictionary::array_bounds_length, array_len) || array_len <= 0) {
                return values;
            }

            // IL2CPP Entry structure layout:
            // struct Entry { int32_t hashCode; int32_t next; int32_t key; int32_t padding; uintptr_t value; };
            // sizeof(Entry) = 24 (0x18)
            constexpr size_t entry_stride = 0x18;
            constexpr size_t value_offset_in_entry = 0x10;

            uintptr_t data_start = entries_array + offsets::il2cpp_dictionary::array_data_start;
            int32_t found = 0;

            for (int32_t i = 0; i < array_len && found < count; ++i) {
                uintptr_t entry_addr = data_start + (static_cast<size_t>(i) * entry_stride);
                int32_t hash_code = 0;
                if (!read(entry_addr, hash_code)) {
                    break;
                }

                if (hash_code >= 0) {
                    uintptr_t val = read_ptr(entry_addr + value_offset_in_entry);
                    if (val > 0x10000) {
                        values.push_back(val);
                        ++found;
                    }
                }
            }

            return values;
        }

        // Parse MoneyWorkbenchModel entry
        bool parse_money_workbench(uintptr_t model_addr, TableRecord& record) const {
            if (!model_addr || model_addr <= 0x10000) {
                return false;
            }

            record.type = TableType::MoneyWorkbench;
            record.address = model_addr;
            record.dirty_cash = 0;
            record.clean_cash = 0;
            record.reward_cash = 0;
            record.total_cash = 0;
            record.yaw = 0.0f;

            // 1. Transform / Spatial Coordinates
            uintptr_t desc_model = read_ptr(model_addr + offsets::money_workbench::description_model);
            if (desc_model > 0x10000) {
                uintptr_t loc_desc = read_ptr(desc_model + offsets::money_workbench::location_description);
                if (loc_desc > 0x10000) {
                    read(loc_desc + offsets::money_workbench::position, record.position);
                    read(loc_desc + offsets::money_workbench::yaw_offset, record.yaw);
                }
            }

            // 2. Cash Values
            uintptr_t data_ptr = read_ptr(model_addr + offsets::money_workbench::data_container);
            if (data_ptr > 0x10000) {
                uintptr_t dirty_data = read_ptr(data_ptr + offsets::money_workbench::dirty_amount_ptr);
                if (dirty_data > 0x10000) {
                    read(dirty_data + offsets::money_workbench::cash_value, record.dirty_cash);
                }

                uintptr_t clean_data = read_ptr(data_ptr + offsets::money_workbench::clean_amount_ptr);
                if (clean_data > 0x10000) {
                    read(clean_data + offsets::money_workbench::cash_value, record.clean_cash);
                }

                record.total_cash = record.dirty_cash + record.clean_cash;
            }

            return true;
        }

        // Parse CollectionTableModel entry
        bool parse_collection_table(uintptr_t model_addr, TableRecord& record) const {
            if (!model_addr || model_addr <= 0x10000) {
                return false;
            }

            record.type = TableType::CollectionTable;
            record.address = model_addr;
            record.dirty_cash = 0;
            record.clean_cash = 0;
            record.reward_cash = 0;
            record.total_cash = 0;
            record.yaw = 0.0f;

            // 1. Transform / Spatial Coordinates
            uintptr_t desc_model = read_ptr(model_addr + offsets::collection_table::description_model);
            if (desc_model > 0x10000) {
                uintptr_t loc_desc = read_ptr(desc_model + offsets::collection_table::location_description);
                if (loc_desc > 0x10000) {
                    read(loc_desc + offsets::collection_table::position, record.position);
                    read(loc_desc + offsets::collection_table::yaw_offset, record.yaw);
                }
            }

            // 2. Reward Cash Value
            uintptr_t reward_model = read_ptr(model_addr + offsets::collection_table::reward_model);
            if (reward_model > 0x10000) {
                uintptr_t reward_data = read_ptr(reward_model + offsets::collection_table::reward_data);
                if (reward_data > 0x10000) {
                    uintptr_t cash_data = read_ptr(reward_data + offsets::collection_table::cash_amount_primary);
                    if (!cash_data || cash_data <= 0x10000) {
                        cash_data = read_ptr(reward_data + offsets::collection_table::cash_amount_secondary);
                    }

                    if (cash_data > 0x10000) {
                        read(cash_data + offsets::collection_table::cash_value, record.reward_cash);
                        record.total_cash = record.reward_cash;
                    }
                }
            }

            return true;
        }

        // Collect all tables active within WorldEntitiesModel
        std::vector<TableRecord> collect_tables_from_world(uintptr_t world_entities) const {
            std::vector<TableRecord> records;
            if (!world_entities || world_entities <= 0x10000) {
                return records;
            }

            // Collect Money Tables
            uintptr_t money_dict = read_ptr(world_entities + offsets::world_entities::money_workbenches_dict);
            if (money_dict > 0x10000) {
                auto money_ptrs = extract_dictionary_values(money_dict);
                for (uintptr_t model_ptr : money_ptrs) {
                    TableRecord record{};
                    if (parse_money_workbench(model_ptr, record)) {
                        records.push_back(record);
                    }
                }
            }

            // Collect Collection Tables
            uintptr_t collection_dict = read_ptr(world_entities + offsets::world_entities::collection_tables_dict);
            if (collection_dict > 0x10000) {
                auto collection_ptrs = extract_dictionary_values(collection_dict);
                for (uintptr_t model_ptr : collection_ptrs) {
                    TableRecord record{};
                    if (parse_collection_table(model_ptr, record)) {
                        records.push_back(record);
                    }
                }
            }

            return records;
        }

        std::vector<TableRecord> collect_tables(uintptr_t battle_model) const {
            uintptr_t world_entities = resolve_world_entities(battle_model);
            return collect_tables_from_world(world_entities);
        }

    private:
        ReadMemoryFn read_memory_;
    };

    // Helper functions for easy consumption across the codebase
    inline bool ReadMoneyWorkbench(uintptr_t model, TableRecord& record, const ReadMemoryFn& read = nullptr) {
        MoneyTableReader reader(read);
        return reader.parse_money_workbench(model, record);
    }

    inline bool ReadCollectionTable(uintptr_t model, TableRecord& record, const ReadMemoryFn& read = nullptr) {
        MoneyTableReader reader(read);
        return reader.parse_collection_table(model, record);
    }

    inline std::vector<TableRecord> CollectTables(uintptr_t battle_model, const ReadMemoryFn& read = nullptr) {
        MoneyTableReader reader(read);
        return reader.collect_tables(battle_model);
    }

} // namespace rob_run
