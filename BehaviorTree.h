#ifndef BEHAVIORTREE_BEHAVIORTREE_H
#define BEHAVIORTREE_BEHAVIORTREE_H

#include <vector>
#include <exception>
#include <string>

/**
 * <h3>Simple Behavior Tree base implementation.</h3>
 * Enforces code separation, clear code structure and modularity.
 * <br/><br/>
 * Enums:
 * <ul>
 * <li>NodeState</li>
 * </ul>
 * Classes:
 * <ul>
 * <li>Node
 * <li>BehaviorTree</li>
 * <li>ISelectorBranch</li>
 * <li>ISequenceBranch</li>
 * <li>IActionLeaf</li>
 * <li>IConditionLeaf</li>
 * </ul>
 */
namespace BHT
{
    enum NodeState
    {
        SUCCESS,
        RUNNING,
        FAILURE
    };


    template<class T>
    /**
     * Base class for any node in the behavior tree.
     * To add custom implementations of nodes, derive from this class.
     * @tparam T Data context class of behavior tree
     */
    class Node {
    public:
        T* context;                  // The data context
        std::string name;            // Name of node (optional)
        Node* parent;                // The parent node
        NodeState state;             // State of the evaluation
        std::vector<Node*> children; // List of child nodes

        /**
         * Construct a node for the behavior tree
         * @param parent The parent node. Set null if root node.
         * @param name The name for the node.
         */
        explicit Node(Node* parent = nullptr, std::string name = ""): state(FAILURE)
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
            for (Node* node: children)
            {
                // Calls child node's destructor
                delete node;
            }
        }

        /**
         * A method to evaluate a node in the behavior tree. Must be implemented.
         * @return The evaluation state of the node
         */
        virtual NodeState evaluate() = 0;

    protected:
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

            for (Node* node: children)
            {
                node->context = this->context;
                node->_propagate_context();
            }
        }
    };


    template<class T>
    /**
     * Used internally by the behavior tree. Attaches the tree to this root and
     * propagates the behavior tree context down.
     * @tparam T Data context class of behavior tree
     */
    class Root: public Node<T> {
    public:
        Node<T>* child;

        /**
         * Construct a node for the behavior tree
         * @param parent The parent node. Set null if root node.
         * @param name The name for the node.
         */
        explicit Root(T context, Node<T> tree): Node<T>(nullptr, "root")
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
            this->state = child->evaluate();
            return this->state;
        }

    protected:
        void _propagate_context() override
        {
            if (this->context == nullptr)
                throw std::runtime_error("Context not initialized.");

            child->context = this->context;
            child->_propagate_context();
        }
    };


    template<class T>
    class BehaviorTree {
    public:
        BehaviorTree(T* context, Node<T> tree)
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
     * <br/><br/>
     * The nodes in the selector's children are iterated over and evaluated by calling <code>evaluate</code>.<br/>
     * These child nodes can be other branches, decorators, or leaf nodes.
     * <ul>
     * <li>If a child node is evaluated to <code>FAILURE</code> the selector continues on to the next child node.</li>
     * <li>The selector returns on the first <code>SUCCESS</code> evaluation of a child with a <code>SUCCESS</code> state.</li>
     * <li>The selector returns on the first <code>RUNNING</code> evaluation of a child with a <code>RUNNING</code> state.</li>
     * </ul>
     *
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class ISelectorBranch: public Node<T> {
    public:
        explicit ISelectorBranch(Node<T>* parent = nullptr, std::string name = ""): Node<T>(parent, name)
        {
            // Triggers recursive tree creation
            construct();
        }

        /**
         * Attach the children to the node within this method.
         * The method is invoked when the class constructor is invoked
         * and triggers a tree creation.
         */
        virtual void construct() = 0;

        NodeState evaluate() override
        {
            for (Node<T>* child: this->children)
            {
                // Assign the new state
                child->state = child->evaluate();
                switch (child->state)
                {
                    // Return first successful child branch
                    case SUCCESS:
                        return SUCCESS;
                        // Return first running child branch
                    case RUNNING:
                        return RUNNING;
                }
            }
            // All sub-branches failed
            return FAILURE;
        }
    };


    /**
     * A sequence tries to evaluate all it's children
     * <br/><br/>
     * The nodes in the sequence's children are iterated over and evaluated by calling <code>evaluate</code>.<br/>
     * These child nodes can be other branches, decorators, or leaf nodes.
     * <ul>
     * <li>If any of the nodes return a <code>FAILURE</code> state, the sequence is aborted and returns <code>FAILURE</code>.</li>
     * <li>If all nodes return a <code>SUCCESS</code> state, the sequence returns <code>SUCCESS</code>.</li>
     * <li>If a node returns <code>RUNNING</code>, the sequence stops and returns <code>RUNNING</code>.</li>
     * </ul>
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class ISequenceBranch: public Node<T> {
    public:
        explicit ISequenceBranch(Node<T>* parent = nullptr, std::string name = ""): Node<T>(parent, name)
        {
            // Triggers recursive tree creation
            construct();
        }

        /**
         * Attach the children to the node within this method. (Using _attach(child))
         * The method is invoked when the class constructor is invoked
         * and triggers a tree creation.
         */
        virtual void construct() = 0;

        NodeState evaluate() override
        {
            // Iterate and evaluate all children
            for (Node<T>* child: this->children)
            {
                // Assign new child state
                child->state = child->evaluate();
                // State of this node is the same as child
                this->state = child->state;
                // Stop iterating if the state isn't success
                if (child->state != SUCCESS)
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
     * <br/><br/>
     * Used to check whether a condition is true or false.
     * <br/>
     * They are similar to action leaves but <b>should not</b> alter the state of the system.<br/>
     * Meaning a condition leaf is always synchronous and does not return a <code>RUNNING</code> state.
     * <br/><br/>
     * Requires an implementation of the condition() method.
     */
    template<class T>
    class IConditionLeaf: public Node<T> {
        NodeState evaluate() override {
            if (condition())
            {
                this->state = SUCCESS;
                return this->state;
            }
            this->state = FAILURE;
            return this->state;
        }
        /**
         * This method describes whether the node's condition is met or not.
         * <br/><br/>
         * If the method returns true, the state will be evaluated to SUCCESS.<br/>
         * If the method returns false, the state will be evaluated to FAILURE.
         * @return true when condition is met. False otherwise.
         */
        virtual bool condition() = 0;
    };

    /**
     * Base class for action leaves. Actions do not have child nodes.
     * <br/><br/>
     * An action leaf performs a task.<br/>
     * The action can return in a <code>SUCCESS</code>, <code>FAILURE</code> or <code>RUNNING</code> state.
     * @tparam T Data context class of behavior tree
     */
    template<class T>
    class IActionLeaf: public Node<T> {

        NodeState evaluate() override {
            return action();
        }

        /**
         * The task of the action node
         * @return The state of the action at the time of returning
         */
        virtual NodeState action() = 0;
    };

}

#endif //BEHAVIORTREE_BEHAVIORTREE_H
