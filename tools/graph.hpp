//
// tools/graph.hpp
//
// Контейнер ориентированного графа.
// Основная задача — хранить узлы и дуги, давать удобный доступ
// к ним и обеспечивать устойчивость итераторов при модификации.
//

#ifndef LAB1_GRAPH_HPP
#define LAB1_GRAPH_HPP

#include <algorithm>
#include <iterator>
#include <list>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace graph {

// ===========================================================================
// checker: вспомогательные шаблоны, не являющиеся частью публичного API
// ===========================================================================
namespace checker {

/**
 * @brief Хранилище пользовательских данных узла/дуги.
 *
 * Для непустого типа T содержит поле `data` типа T.
 * Специализация для void не хранит ничего.
 *
 * @tparam T тип пользовательских данных; допускается void.
 */
template <typename T>
struct data_holder {
    T data;

    data_holder() = default;

    /// Конструирует данные из переданных аргументов.
    template <typename... Args>
    explicit data_holder(Args&&... args)
        : data(std::forward<Args>(args)...) {}
};

/// Специализация для случая, когда данные не нужны.
template <>
struct data_holder<void> {
    data_holder() = default;

    /// Аргументы игнорируются — хранить их негде.
    template <typename... Args>
    explicit data_holder(Args&&...) {}
};

/**
 * @brief Проверка, что тип T годится для хранения в узле/дуге.
 *
 * Допустимы либо void, либо копируемо-присваиваемый тип.
 * Используется в enable_if на самом классе orgraph_t.
 */
template <typename T>
constexpr bool valid_data =
    std::is_void_v<T> ||
    (std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>);

} // namespace checker

// ---------------------------------------------------------------------------
// GRAPH_ASSERT
//
// Внутренний макрос проверок инвариантов. Под DEBUG при нарушении условия
// бросает std::logic_error с указанным сообщением. Под релизом — ничего
// не делает (компилятор выкидывает весь код под ним).
//
// Используется только в validate(); вне класса не предназначен.
// ---------------------------------------------------------------------------
#ifdef DEBUG
    #define GRAPH_ASSERT(cond, msg) \
        do { if (!(cond)) throw std::logic_error(msg); } while (0)
#else
    #define GRAPH_ASSERT(cond, msg) do {} while (0)
#endif

/**
 * @brief Ориентированный граф с пользовательскими данными в узлах и дугах.
 *
 * Хранит все узлы и дуги в std::list, что обеспечивает:
 *   - стабильные итераторы на узлы/дуги при вставке/удалении других
 *     элементов (инвалидируются только итераторы на удалённый элемент);
 *   - отсутствие массовой инвалидации при добавлении.
 *
 * Структура ссылок:
 *   - node_t хранит список итераторов на входящие и исходящие дуги;
 *   - edge_t хранит итераторы на предка (source) и потомка (target).
 *
 * Инварианты (проверяются в validate() под DEBUG):
 *   1. Каждая дуга из edges_ зарегистрирована в списках in_/out_ своих
 *      концов.
 *   2. Каждая дуга, лежащая в in_/out_ узла, действительно указывает
 *      на этот узел своим source_/target_.
 *
 * @tparam Tnode_data тип пользовательских данных узла (возможно void).
 * @tparam Tedge_data тип пользовательских данных дуги (возможно void).
 */
template <typename Tnode_data, typename Tedge_data,
          typename = std::enable_if_t<
              checker::valid_data<Tnode_data> &&
              checker::valid_data<Tedge_data>>>
class orgraph_t {
public:
    class node_t;
    class edge_t;

    /// Итератор на узел (неконстантный).
    using node_it       = typename std::list<node_t>::iterator;
    /// Итератор на узел (константный).
    using const_node_it = typename std::list<node_t>::const_iterator;
    /// Итератор на дугу (неконстантный).
    using edge_it       = typename std::list<edge_t>::iterator;
    /// Итератор на дугу (константный).
    using const_edge_it = typename std::list<edge_t>::const_iterator;

    // -----------------------------------------------------------------------
    // node_t
    // -----------------------------------------------------------------------

    /**
     * @brief Узел графа.
     *
     * Хранит пользовательские данные и списки итераторов на инцидентные
     * дуги. Прямое изменение этих списков снаружи запрещено — только
     * через методы orgraph_t (класс объявлен friend).
     */
    class node_t {
        checker::data_holder<Tnode_data> node_data_;
        std::list<edge_it> in_;    ///< входящие дуги (итераторы в edges_)
        std::list<edge_it> out_;   ///< исходящие дуги (итераторы в edges_)

        friend class orgraph_t;

    public:
        /// Создаёт узел, конструируя данные из переданных аргументов.
        template <typename... Args>
        explicit node_t(Args&&... args)
            : node_data_(std::forward<Args>(args)...) {}

        /**
         * @brief Доступ к пользовательским данным узла.
         * @note Метод отсутствует (SFINAE), если Tnode_data == void.
         */
        template <typename T = Tnode_data,
                  typename = std::enable_if_t<!std::is_void_v<T>>>
        T& get_data() { return node_data_.data; }

        /// Константная версия get_data().
        template <typename T = Tnode_data,
                  typename = std::enable_if_t<!std::is_void_v<T>>>
        const T& get_data() const { return node_data_.data; }

        /// Список итераторов на входящие дуги (только чтение).
        const std::list<edge_it>& get_in()  const noexcept { return in_; }

        /// Список итераторов на исходящие дуги (только чтение).
        const std::list<edge_it>& get_out() const noexcept { return out_; }
    };

    // -----------------------------------------------------------------------
    // edge_t
    // -----------------------------------------------------------------------

    /**
     * @brief Дуга графа.
     *
     * Хранит пользовательские данные и итераторы на узлы-концы:
     * source_ (откуда) и target_ (куда). Изменять концы можно только
     * через set_source()/set_target() у графа.
     */
    class edge_t {
        checker::data_holder<Tedge_data> edge_data_;
        node_it source_;   ///< узел-предок (откуда)
        node_it target_;   ///< узел-потомок (куда)

        friend class orgraph_t;

    public:
        /// Создаёт дугу, конструируя данные из переданных аргументов.
        /// Концы (source_/target_) выставляются отдельно в add_edge().
        template <typename... Args>
        explicit edge_t(Args&&... args)
            : edge_data_(std::forward<Args>(args)...) {}

        /**
         * @brief Доступ к пользовательским данным дуги.
         * @note Метод отсутствует (SFINAE), если Tedge_data == void.
         */
        template <typename T = Tedge_data,
                  typename = std::enable_if_t<!std::is_void_v<T>>>
        T& get_data() { return edge_data_.data; }

        /// Константная версия get_data().
        template <typename T = Tedge_data,
                  typename = std::enable_if_t<!std::is_void_v<T>>>
        const T& get_data() const { return edge_data_.data; }

        /// Итератор на узел-предок.
        node_it source() const noexcept { return source_; }

        /// Итератор на узел-потомок.
        node_it target() const noexcept { return target_; }
    };

    // -----------------------------------------------------------------------
    // Конструкторы / деструктор
    // -----------------------------------------------------------------------

    /// Создаёт пустой граф.
    orgraph_t() = default;

    /// Виртуальный — чтобы наследники (например, tree_t) корректно
    /// удалялись через указатель на orgraph_t.
    virtual ~orgraph_t() = default;

    // -----------------------------------------------------------------------
    // Доступ к контейнерам
    // -----------------------------------------------------------------------

    /// Все узлы графа (для чтения).
    const std::list<node_t>& nodes() const noexcept { return nodes_; }
    /// Все узлы графа (для изменения).
    std::list<node_t>& nodes()       noexcept { return nodes_; }

    /// Все дуги графа (для чтения).
    const std::list<edge_t>& edges() const noexcept { return edges_; }
    /// Все дуги графа (для изменения).
    std::list<edge_t>& edges()       noexcept { return edges_; }

    // ---- итераторы по узлам ----

    node_it       nodes_begin()       noexcept { return nodes_.begin(); }
    node_it       nodes_end()         noexcept { return nodes_.end();   }
    const_node_it nodes_begin() const noexcept { return nodes_.begin(); }
    const_node_it nodes_end()   const noexcept { return nodes_.end();   }

    // ---- итераторы по дугам ----

    edge_it       edges_begin()       noexcept { return edges_.begin(); }
    edge_it       edges_end()         noexcept { return edges_.end();   }
    const_edge_it edges_begin() const noexcept { return edges_.begin(); }
    const_edge_it edges_end()   const noexcept { return edges_.end();   }

    // -----------------------------------------------------------------------
    // Модификация
    // -----------------------------------------------------------------------

    /**
     * @brief Создаёт новый узел без инцидентных дуг.
     *
     * Аргументы передаются в конструктор данных узла.
     *
     * @return итератор на созданный узел.
     * @note Итераторы на существующие узлы не инвалидируются.
     */
    template <typename... Args>
    node_it add_node(Args&&... args) {
        nodes_.emplace_back(std::forward<Args>(args)...);
        node_it n = std::prev(nodes_.end());
        validate();
        return n;
    }

    /**
     * @brief Создаёт дугу from -> to.
     *
     * Аргументы передаются в конструктор данных дуги. Дуга регистрируется
     * в списках out_ предка и in_ потомка.
     *
     * @param from итератор на узел-предок.
     * @param to   итератор на узел-потомок.
     * @return итератор на созданную дугу.
     * @note Существующие узлы и дуги не инвалидируются.
     */
    template <typename... Args>
    edge_it add_edge(node_it from, node_it to, Args&&... args) {
        edges_.emplace_back(std::forward<Args>(args)...);
        edge_it e = std::prev(edges_.end());

        e->source_ = from;
        e->target_ = to;

        from->out_.push_back(e);
        to->in_.push_back(e);

        validate();
        return e;
    }

    /**
     * @brief Меняет узел-предок дуги.
     *
     * Убирает дугу из списка исходящих старого предка и добавляет
     * в список исходящих нового.
     *
     * @param edg        итератор на дугу.
     * @param new_source итератор на нового предка.
     */
    void set_source(edge_it edg, node_it new_source) {
        edg->source_->out_.remove(edg);
        edg->source_ = new_source;
        new_source->out_.push_back(edg);
        validate();
    }

    /**
     * @brief Меняет узел-потомок дуги.
     *
     * Убирает дугу из списка входящих старого потомка и добавляет
     * в список входящих нового.
     *
     * @param edg        итератор на дугу.
     * @param new_target итератор на нового потомка.
     */
    void set_target(edge_it edg, node_it new_target) {
        edg->target_->in_.remove(edg);
        edg->target_ = new_target;
        new_target->in_.push_back(edg);
        validate();
    }

    /**
     * @brief Удаляет дугу.
     *
     * Убирает её из списков инцидентности обоих узлов и из edges_.
     * Узлы не трогаются.
     *
     * @param e итератор на удаляемую дугу.
     * @note После вызова e инвалидирован.
     */
    void erase_edge(edge_it e) {
        e->source_->out_.remove(e);
        e->target_->in_.remove(e);
        edges_.erase(e);
        validate();
    }

    /**
     * @brief Удаляет узел вместе со всеми инцидентными дугами.
     *
     * Порядок важен: сначала удаляются исходящие дуги, потом входящие.
     * Мы итерируемся по n->out_ и n->in_, но модифицируем только чужие
     * списки (у противоположных концов) и edges_. Свои списки n->out_/
     * n->in_ не трогаем до конца — иначе итераторы сломаются.
     *
     * @param n итератор на удаляемый узел.
     * @note После вызова n и итераторы на инцидентные дуги инвалидированы.
     */
    void erase_node(node_it n) {
        // 1. удаляем все исходящие дуги
        for (edge_it e : n->out_) {
            e->target_->in_.remove(e);
            edges_.erase(e);
        }
        // 2. удаляем все входящие дуги
        for (edge_it e : n->in_) {
            e->source_->out_.remove(e);
            edges_.erase(e);
        }
        // 3. удаляем сам узел
        nodes_.erase(n);
        validate();
    }

    // -----------------------------------------------------------------------
    // Проверка корректности
    // -----------------------------------------------------------------------

    /**
     * @brief Проверяет инварианты графа.
     *
     * Под DEBUG бросает std::logic_error при нарушении. Под релизом —
     * пустая функция (тело вырезано препроцессором).
     *
     * Проверяется:
     *   1. Каждая дуга из edges_ есть в списках out_ предка и in_ потомка.
     *   2. Каждая дуга из in_/out_ узла действительно указывает на него
     *      своим source_/target_.
     *
     * @note Вызывается автоматически после каждой мутации.
     */
    void validate() const {
#ifdef DEBUG
        // 1. Каждая дуга зарегистрирована у своего предка и потомка.
        for (auto it = edges_.begin(); it != edges_.end(); ++it) {
            const edge_t& e = *it;

            const auto& outs = e.source_->out_;
            GRAPH_ASSERT(std::find(outs.begin(), outs.end(), it) != outs.end(),
                         "edge is missing from source's out list");

            const auto& ins = e.target_->in_;
            GRAPH_ASSERT(std::find(ins.begin(), ins.end(), it) != ins.end(),
                         "edge is missing from target's in list");
        }

        // 2. Каждая дуга в in_/out_ узла действительно указывает на этот узел.
        for (auto nit = nodes_.begin(); nit != nodes_.end(); ++nit) {
            for (edge_it e : nit->in_) {
                GRAPH_ASSERT(e->target_ == nit,
                             "in-edge points to wrong node");
            }
            for (edge_it e : nit->out_) {
                GRAPH_ASSERT(e->source_ == nit,
                             "out-edge points to wrong node");
            }
        }
#endif
    }

private:
    std::list<node_t> nodes_;   ///< все узлы графа
    std::list<edge_t> edges_;   ///< все дуги графа
};

} // namespace graph

#endif // LAB1_GRAPH_HPP