/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        XElement.h

    \brief       Contains the class definition for Xml Utilities
    
    \date        01-13-11 15:58:46
   
*/
/*----------------------------------------------------------------------------*/

#ifndef XELEMENT_H
#define XELEMENT_H

#include <string>
#include <vector>
#include <map>
#include <scxcorelib/scxhandle.h>


namespace SCXCoreLib
{
    namespace Xml
    {
                // Forward declaring to enable XElementPr typedef
                class XElement;
                
                /*----------------------------------------------------------------------------*/
                /**
                    Represents a XElement Safe Pointer

                    \date   01-14-11 09:19:18
                    
                    auto_ptr is not safe to be used in STL, so using ScxHandle
                */
                typedef SCXCoreLib::SCXHandle<XElement> XElementPtr;
                
                /*----------------------------------------------------------------------------*/
                /**
                    Represents a XElement Safe Pointer Vector

                    \date   01-14-11 09:19:18
                    
                */
                typedef std::vector<XElementPtr> XElementList;
                
                
                /*----------------------------------------------------------------------------*/
                /**
                    Represents a XML Element for processing and creating XML

                    \date   01-13-11 16:01:13
                    
                    Most the requirements for the XML processing for the client requires a lot of
                    in memory processing. In memory processing are applicable for small XML files.
                    
                    \warning There are two outstanding bugs that will be fixed in the future
                                \li The static method Load needs to be made thread safe
                                \li Protect against adding looping children
                */
                class XElement
                {
                    public:
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Create a XElement object with name
                            
                            \param [in] name of the Element
                            
                        */
                        XElement(const std::string& name);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Create a XElement object with name
                            
                            \param [in] name Name of the element
                            \param [in] content Content text of the element
                            
                        */
                        XElement(const std::string& name, const std::string& content);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Virtual destructor for XElement
                        */
                        virtual ~XElement();
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get the name of the element
                            
                            \return Name of the element
                        */
                        std::string GetName() const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get the content text of the element
                            
                            \return Content text of the element
                        */
                        std::string GetContent() const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Set the content text of the element
                            
                            \param [in] Content text of the element
                        */
                        void SetContent(const std::string& content);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Add a child to the element
                            
                            \param [in] child Pointer to Element to be added as a child
                        */
                        void AddChild(const XElementPtr child);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get the first child with name
                            
                            \param [in] name Name of the child to search
                            \param [out] child Child Element Pointer
                            \return Return true if a child with the name was found
                            
                            If tranversing a element with the root child having multiple siblings
                            use GetChildren. 
                            \warning The returned child is editable. 
                        */
                        bool GetChild(const std::string& name, XElementPtr& child) const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get all the children of the element
                            
                            \param [out] childElements Vector of all child elements
                        */
                        void GetChildren(XElementList& childElements) const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Set the value of an attribute. If a particular attribute name is not found.
                            the attribute is added.
                            
                            \param [in] name Name of the attribute
                            \param [in] vale Value of the attribute
                        */
                        void SetAttributeValue(const std::string& name, const std::string& value);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get the value of the attribute. If the value is not found a false is returned
                            
                            \param [in] name Name of the attribute
                            \param [out] value Value of the attribute
                            \return true if the attribute is present
                        */
                        bool GetAttributeValue(const std::string& name, std::string& value) const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Get the attribute map of the element
                            
                            \param [out] attributeMap map of the element
                        */
                        void GetAttributeMap(std::map<std::string, std::string>& attributeMap) const;
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Load a XML string into the XElement
                            
                            \param [in] xmlString String representation of an Xml 
                            \param [out] element The loaded xml's root element
                            
                            This is a static method and is thread safe. The thread safety might cause the
                            loading of "large" xml files slow.
                        */
                        static void Load(const std::string& xmlString, XElementPtr& element);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Save the XElement as Xml String
                            
                            \param [out] xmlString String representation of an Xml 
                            \param [in] enableLineSeperators Enable insertion of new lines between elements
                        */
                        void ToString(std::string& xmlString, bool enableLineSeperators);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Select all nodes that match the given XPATH query
                            
                            \param [int] xpath XPATH query to execute
                            \param [out] selectionList List of XElementPtr that matches the query
                            \return true if the selection yeilded atleast one element
                            
                            \warning NOT IMPLEMENTED
                        */
                        bool SelectNodes(const std::string& xpath, XElementList selectionList);
                        
                        /*----------------------------------------------------------------------------*/
                        /**
                            Select the first that matches the given XPATH query
                            
                            \param [int] xpath XPATH query to execute
                            \param [out] selection The first XElementPtr that matches the query
                            \return true if the selection yeilded one element
                            
                            \warning NOT IMPLEMENTED
                        */
                        bool SelectSingleNode(const std::string& xpath, XElementPtr selection);
                        
                        private:
                            /** The name of the XElement */
                            std::string m_name;
                            
                            /** The content string of the XElement */
                            std::string m_content;
                            
                            /** The vector of child element pointers */
                            XElementList m_childList;
                            
                            /** The map of attribute names and values */
                            std::map<std::string, std::string> m_attributeMap;
                            
                            
                            class XmlWriterImpl; // Forward declaration to hide implementation
                            
                            /** Pointer to the XmlWriter Implmentation */
                            XmlWriterImpl* m_writer;
                            // Note: auto_ptr could not be used for the above. As its desctructor will not be called
                            
                            /*----------------------------------------------------------------------------*/
                            /**
                                Hiding the copy constructor to save unintended consequence of copy STL 
                                container members.
                            */
                            XElement(const XElement& element);
                            
                            /*----------------------------------------------------------------------------*/
                            /**
                                Set the name of the XElement
                                
                                /param [in] name  Name of the XElement
                            */
                            void SetName(const std::string& name);
                            
                            /*----------------------------------------------------------------------------*/
                            /**
                                Validate if the name is a valid XML Element\Attribute name
                                
                                /param [in] name Name to validate
                                /return true if the passed name is a valid XML name
                            */
                            bool IsValidName(const std::string& name) const;
                            
                            /*----------------------------------------------------------------------------*/
                            /**
                                Add the element to XML Writer
                                
                                /param [in] element  element add to the Writer
                            */
                            void AddToWriter(XElement* element);
                            
                            /** 
                             * Constants
                             */
                             
                             static const std::string EXCEPTION_MESSAGE_EMPTY_NAME;
                             static const std::string EXCEPTION_MESSAGE_NULL_CHILD;
                             static const std::string EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME;
                             static const std::string EXCEPTION_MESSAGE_INPUT_EMPTY;
                             static const std::string EXCEPTION_MESSAGE_INVALID_NAME;
                    };
                
                
                
                /*----------------------------------------------------------------------------*/
                /**
                    Base class for all Xml Parsing \ Building Exceptions

                    \date   01-14-11 09:37:53
                */
                class XmlException : public std::exception
                {
                    public:
                        XmlException(const std::string message) : m_message(message) {}
                        
                        ~XmlException() throw() {}
                        
                        virtual const char* what() const throw()
                        {
                            return m_message.c_str();
                        }

                    private:
                        std::string m_message;
                };
    }
}
#endif /* XELEMENT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
