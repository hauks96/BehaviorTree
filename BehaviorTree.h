#ifndef BEHAVIORTREE_BEHAVIORTREE_H
#define BEHAVIORTREE_BEHAVIORTREE_H

#include <vector>
#include <exception>
#include <string>

/**
 * Simple Behavior Tree base implementation
 * Enforces code separation, clear code structure and modularity.
 *
 * Enums:
 * - NodeState
 *
 * Classes:
 * - Node
 * - BehaviorTree
 * - ISelectorBranch
 * - ISequenceBranch
 * - IActionLeaf
 * - IConditionLeaf
 */
namespace BHT
{
    enum class NodeState
    {
        SUCCESS,
        RUNNING,
        FAILURE
    };


    /**
     * Base class for any node in the behavior tree.
     * To add custom implementations of nodes, derive from this class.
     *
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class Node {
    public:
        T* context;                  // The data context
        std::string name;            // Name of node (optional)
        Node* parent;                // The parent node
        NodeState state;             // State of the evaluation
        std::vector<Node*> children; // List of child nodes
        bool DEBUG = false;          // Print the evaluation if true

        /**
         * Construct a node for the behavior tree
         *
         * @param parent The parent node. Set null if root node.
         * @param name The name for the node.
         */
        explicit Node(Node* parent = nullptr, std::string name = "") : state(NodeState::FAILURE)
        {
            this->parent = parent;
            this->name = name;
            this->context = nullptr;
        }

        /**
         * Recursively destroys entire tree
         */
        ~Node()
        {
            // Iterate all child nodes and delete
            for (Node* node : children)
            {
                // Calls child node's destructor
                delete node;
            }
        }
        void printState()
        {
            switch (this->state)
            {
            case NodeState::SUCCESS:
                std::cout << name << ": SUCCESS" << std::endl;
                break;
            case NodeState::FAILURE:
                std::cout << name << ": FAILURE" << std::endl;
                break;
            case NodeState::RUNNING:
                std::cout << name << ": RUNNING" << std::endl;
                break;
            }
        }

        NodeState eval()
        {
            NodeState evalState = evaluate();
            if (this->DEBUG) printState();
            return evalState;
        }

        /**
         * A method to evaluate a node in the behavior tree. Must be implemented.
         * @return The evaluation state of the node
         */
        virtual NodeState evaluate() = 0;

        /**
         * Attach a child node.
         * @param node The node to attach
         */
        virtual void _attach(Node* node)
        {
            node->parent = this;
            this->children.push_back(node);
        }

        /**
         * Call from root node to propagate the context down the entire tree
         * @throws runtime_error if the context parameter is nullprt
         */
        virtual void _propagate_context()
        {
            if (this->context == nullptr)
                throw std::runtime_error("Context not initialized");

            for (Node* node : children)
            {
                if (this->DEBUG) node->DEBUG = true;
                node->context = this->context;
                node->_propagate_context();
            }
        }
    };


    /**
     * Used internally by the behavior tree. Attaches the tree to this root and
     * propagates the behavior tree context down.
     *
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class Root : public Node<T>
    {
    public:
        Node<T>* child;

        /**
         * Construct a node for the behavior tree
         *
         * @param parent The parent node. Set null if root node.
         * @param name The name for the node.
         */
        explicit Root(T* context, Node<T>* tree) : Node<T>(nullptr, "root")
        {
            child = tree;
            this->context = context;
            _propagate_context();
        }

        ~Root()
        {
            delete child;
        }

        NodeState evaluate() override
        {
            this->state = child->eval();
            return this->state;
        }

        void _propagate_context() override
        {
            if (this->context == nullptr)
                throw std::runtime_error("Context not initialized.");

            child->context = this->context;
            child->_propagate_context();
        }
    };


    template<class T>
    class BehaviorTree
    {
    public:
        BehaviorTree(T* context, Node<T>* tree)
        {
            _root = new Root<T>(context, tree);
        }

        ~BehaviorTree()
        {
            delete this->root;
        }

        /**
         * Performs an iteration of the behavior tree
         */
        void Update()
        {
            _root->evaluate();
        }

    private:
        Root<T>* _root;
    };


    /**
     * Selector branches 'select' the first sub-branch (child node) that returns SUCCESS or RUNNING.
     * The evaluation order is from left to right in the list of children the Selector holds.
     *
     * The nodes in the selector's children are iterated over and evaluated by calling evaluate.
     * These child nodes can be other branches, decorators, or leaf nodes.
     *
     * - If a child node is evaluated to FAILURE the selector continues on to the next child node.
     * - The selector returns on the first SUCCESS evaluation of a child with a SUCCESS state.
     * - The selector returns on the first RUNNING evaluation of a child with a RUNNING state.
     *
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class ISelectorBranch : public Node<T>
    {
    public:
        /**
         * Attach the children to the node within the constructor using the _attach(child*) method
         */
        explicit ISelectorBranch(Node<T>* parent = nullptr, std::string name = "") : Node<T>(parent, name)
        {}

        NodeState evaluate() override
        {
            for (Node<T>* child : this->children)
            {
                // Assign the new state
                child->state = child->eval();
                this->state = child->state;
                switch (child->state)
                {
                    // Return first successful child branch
                case NodeState::SUCCESS:
                    return NodeState::SUCCESS;
                    // Return first running child branch
                case NodeState::RUNNING:
                    return NodeState::RUNNING;
                }
            }
            // All sub-branches failed
            return NodeState::FAILURE;
        }
    };


    /**
     * A sequence tries to evaluate all it's children
     *
     * The nodes in the sequence's children are iterated over and evaluated by calling evaluate.
     * These child nodes can be other branches, decorators, or leaf nodes.
     *
     * - If any of the nodes return a FAILURE state, the sequence is aborted and returns FAILURE.
     * - If all nodes return a SUCCESS state, the sequence returns SUCCESS.
     * - If a node returns RUNNING, the sequence stops and returns RUNNING.
     *
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class ISequenceBranch : public Node<T> {
    public:
        /**
         * Attach the children to the node within the constructor using the _attach(child*) method
         */
        explicit ISequenceBranch(Node<T>* parent = nullptr, std::string name = "") : Node<T>(parent, name)
        {}

        NodeState evaluate() override
        {
            // Iterate and evaluate all children
            for (Node<T>* child : this->children)
            {
                // Assign new child state
                child->state = child->eval();
                // State of this node is the same as child
                this->state = child->state;
                // Stop iterating if the state isn't success
                if (child->state != NodeState::SUCCESS)
                {
                    return child->state;
                }
            }
            // Return Success
            return this->state;
        }
    };

    /**
     * Base class for condition leaves. Conditions do not have child nodes.
     *
     * Used to check whether a condition is true or false.
     *
     * They are similar to action leaves but should not alter the state of the system.
     * Meaning a condition leaf is always synchronous and does not return a RUNNING state.
     *
     * Requires an implementation of the condition() method.
     */
    template<class T>
    class IConditionLeaf : public Node<T>
    {
    public:
        IConditionLeaf(Node<T>* parent = nullptr, std::string name = "") : Node<T>(parent, name){}

        NodeState evaluate() override {
            if (condition())
            {
                this->state = NodeState::SUCCESS;
                return this->state;
            }
            this->state = NodeState::FAILURE;
            return this->state;
        }
        /**
         * This method describes whether the node's condition is met or not.
         *
         * If the method returns true, the state will be evaluated to SUCCESS.
         * If the method returns false, the state will be evaluated to FAILURE.
         * @return true when condition is met. False otherwise.
         */
        virtual bool condition() = 0;
    };

    /**
     * Base class for action leaves. Actions do not have child nodes.
     *
     * An action leaf performs a task.
     * The action can return in a SUCCESS, FAILURE or RUNNING state.
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class IActionLeaf : public Node<T>
    {
    public:
        IActionLeaf(Node<T>* parent = nullptr, std::string name = ""): Node<T> (parent, name) {}

        NodeState evaluate() override {
            return action();
        }

        /**
         * The task of the action node
         * @return The state of the action at the time of returning
         */
        virtual NodeState action() = 0;
    };

    /**
     * All child nodes are evaluated until a FAILURE evaluation
     * Evaluates to SUCCESS if all children retrun success
     * Evaluates to RUNNING if any of the children returns running
     * Evaluates to FAILURE if any of the nodes return failure
     */
    template<class T>
    class IParalellSequence : public Node<T>
    {
    public:
        /**
         * Attach the children to the node within this method. (Using _attach(child))
         */
        IParalellSequence(Node<T>* parent = nullptr, std::string name = "") : Node<T>(parent, name)
        {};

        NodeState evaluate() override {
            bool allSuccess = true;
            // Iterate and evaluate all children
            for (Node<T>* child : this->children)
            {
                // Assign new child state
                child->state = child->eval();
                // State of this node is the same as child
                this->state = child->state;
                // Stop iterating if the state isn't success
                if (child->state == NodeState::FAILURE) return child->state;
                if (child->state == NodeState::RUNNING) allSuccess = false;
            }
            if (!allSuccess) {
                this->state = NodeState::RUNNING;
                return NodeState::RUNNING;
            }
            this->state = NodeState::SUCCESS;
            return NodeState::SUCCESS;
        }
    };

    template<class T>
    class IDecorator : public Node<T> {
    public:
        IDecorator(Node<T>* child) : Node<T>(nullptr, "Decorator")
        {
            this->child = child;
            child->parent = this;
        }

        NodeState evaluate() override
        {
            return decorate(child.eval());
        }

        void _propagate_context() override
        {
            child->context = this->context;
            child->DEBUG = this->DEBUG;
            child->_propagate_context();
        }

        virtual NodeState decorate(NodeState childEvaluation) = 0;

        Node<T>* child;
    };

    template<class T>
    class Inverter : public IDecorator<T> {
    public:
        Inverter(Node<T>* child) : IDecorator<T>(child)
        {}
        NodeState decorate(NodeState childEvaluation) override
        {
            if (childEvaluation == NodeState::SUCCESS) return NodeState::FAILURE;
            if (childEvaluation == NodeState::FAILURE) return NodeState::SUCCESS;
            if (childEvaluation == NodeState::RUNNING) return NodeState::RUNNING;
        }
    };

}

#endif //BEHAVIORTREE_BEHAVIORTREE_H
