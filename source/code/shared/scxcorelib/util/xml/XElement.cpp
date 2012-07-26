#include <scxcorelib/XElement.h>
#include <scxcorelib/nw/xmlcxx.h>
#include <stack>
#include <assert.h>
#include <cctype>
#include <algorithm>

using SCXCoreLib::Xml::XElement;
using SCXCoreLib::Xml::XElementPtr;
using SCXCoreLib::Xml::XmlException;

const std::string XElement::EXCEPTION_MESSAGE_EMPTY_NAME            = "The Element name is empty";
const std::string XElement::EXCEPTION_MESSAGE_NULL_CHILD            = "The child is null";
const std::string XElement::EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME    = "The Attribute name cannot be negative";
const std::string XElement::EXCEPTION_MESSAGE_INPUT_EMPTY            = "The input xml string is empty";
const std::string XElement::EXCEPTION_MESSAGE_INVALID_NAME            = "The name is not valid XML name";

class XElement::XmlWriterImpl
{
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Default constructor
        */
        XmlWriterImpl()
        {
            std::auto_ptr<xmlcxx::XMLWriter> tmp(new xmlcxx::XMLWriter());
            m_writer = tmp;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            Member access operator to access the writer directly
    
            \returns     Pointer to the writer
            
            This is just to increase readability of the code
            So we can write (*m_writer)->DoSomething() than m_writer->g
        */
        xmlcxx::XMLWriter* operator->() const
        {
            return m_writer.get();
        }
        
    private:
        /** Actual XML Writer */
        std::auto_ptr<xmlcxx::XMLWriter> m_writer;
};

/*----------------------------------------------------------------------------*/
/**
    Validate if the given character is valid XML Name first character
    
    /param [in] c character to validate
    /return true if it is a valid XML Name first character
     NameStartChar = [_A-Za-z]
*/
static inline bool IsNameStartChar(char c)
{
    return ((isalpha(c) || c == '_'));
}


/*----------------------------------------------------------------------------*/
/**
    Validate if the given character is valid XML Name non first character
    
    /param [in] c character to validate
    /return true if it is a valid XML Name non first character
    NameChar      = [_A-Za-z\-.0-9]
*/
static inline bool IsNameChar(char c)
{
    return ((IsNameStartChar(c) || isdigit(c) || c == '-' || c =='.' || c == ':' ));
}

// reference http://www.w3.org/TR/REC-xml/#NT-NameStartChar
// NameStartChar = [_A-Za-z]
// NameChar      = [_A-Za-z\-.0-9]
// returning false for all the Extended ASCII characters. As we are working only with < 127
bool XElement::IsValidName(const std::string& name) const
{
    if (name.empty())
    {
        // I think that, by definition, an empty name is pretty much invalid.
        return false;
    }

    const char* nameStr = name.c_str();
    
    // Check if the first character is valid
    if (!IsNameStartChar(*nameStr))
    {
        // First character is not valid return false
        return false;
    }
    else
    {
        nameStr++;
        while(*nameStr)
        {
            // Break on first invalid character
            if (!IsNameChar(*nameStr))
            {
                return false;
            }
            nameStr++;
        }
    }
    // All the characters have been verified return true
    return true;
}

void XElement::SetName(const std::string& name)
{
    if (name.empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_EMPTY_NAME);
    }
    
    if (IsValidName(name))
    {
        m_name = name;
    }
    else
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INVALID_NAME);
    }
}


XElement::XElement(const std::string& name) : m_writer(NULL)
{
    SetName(name);
}

XElement::XElement(const std::string& name, const std::string& content) : m_writer(NULL)
{
    SetName(name);
    SetContent(content);
    SetContent(content);
} 

XElement::~XElement() 
{}

std::string XElement::GetName() const
{
    return m_name;
}

std::string XElement::GetContent() const
{
    return m_content;
}

void XElement::SetContent(const std::string& content)
{
    m_content = content;
}

void XElement::AddChild(const XElementPtr child)
{
    if (child == NULL)
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_NULL_CHILD);
    }
    m_childList.push_back(child);
}

void XElement::GetChildren(XElementList& childElements) const
{
    childElements = m_childList;
}

bool XElement::GetChild(const std::string& name, XElementPtr& child) const
{
    if (!name.empty())
    {
        XElementList::const_iterator it;
        for (it = m_childList.begin(); it != m_childList.end(); ++it)
        {
            if ((*it)->GetName() == name)
            {
                child = *it;
                return true;
            }
        }
    }
    
    // If we are here. No children have been found return false;
    return false;
}

void XElement::SetAttributeValue(const std::string& name, const std::string& value)
{
    if (name.empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_EMPTY_ATTRIBUTE_NAME);
    }
    
    if (IsValidName(name))
    {
        // Insert or update
        // Using access operator
        m_attributeMap[name] = value;
    }
    else
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INVALID_NAME);
    }
}

bool XElement::GetAttributeValue(const std::string& name, std::string& value) const
{
    std::map<std::string, std::string>::const_iterator it;
    
    // if name is empty. It wil automatically return false;
    it = m_attributeMap.find(name);
    
    if (it != m_attributeMap.end())
    {
        value = it->second;
        return true;
    }
    
    // If we are here the attribute has not been found return false
    return false;
}


void XElement::GetAttributeMap(std::map<std::string, std::string>& attributeMap) const
{
    attributeMap = m_attributeMap;
}

void XElement::AddToWriter(XElement* element)
{
    // The writer has to be instantiated before this.
    assert(m_writer != NULL);
    
    // Get the List of Attributes and add it to the writer
    std::map<std::string, std::string> attributeMap;
    element->GetAttributeMap(attributeMap);
    
    if (attributeMap.size() != 0)
    {
        // create a vector of the same size of the map
        std::vector<xmlcxx::XMLAttr> attributeList;
        attributeList.reserve(attributeMap.size());
        
        std::map<std::string, std::string>::const_iterator it = attributeMap.begin();
        while(it != attributeMap.end())
        {
            attributeList.push_back(xmlcxx::XMLAttr(it->first, it->second));
            
            // increment the iterator
            ++it;
        }
        
        // reverse the attributeList to reverse the effects for push_back 
        // else the order will be reversed
        // this is just a cosmetic fix 
        std::reverse(attributeList.begin(), attributeList.end());

        // Write the start tag with the attributes
        std::string tag = element->GetName();
        (*m_writer)->PutStartTag(tag.empty() ? "" : tag.c_str(), attributeList); 
    }
    else
    {
        // Add the start tag without attributes
        std::string tag = element->GetName();
        (*m_writer)->PutStartTag(tag.empty() ? "" : tag.c_str());
    }
    
    // put the content if not empty
    std::string content = element->GetContent();
    if(!content.empty())
    {
        (*m_writer)->PutCharacterData(content.c_str());
    }
    
    // for each child do that same
    std::vector<XElementPtr> childElementList;
    element->GetChildren(childElementList);
    if (childElementList.size() != 0)
    {
        std::vector<XElementPtr>::const_iterator vi = childElementList.begin();
        while (vi != childElementList.end())
        {
            // Call recursive write
            AddToWriter(vi->GetData());
            
            // increment iterator
            ++vi;
        }
    }
    
    // add the end tag
    std::string tag = element->GetName();
    (*m_writer)->PutEndTag(tag.empty() ? "" : tag.c_str());
}

void XElement::ToString(std::string& xmlString, bool enableLineSeperators)
{
    // The writer has to be null
    assert(m_writer == NULL);
    
    m_writer = new XmlWriterImpl();
    
    if (enableLineSeperators)
    {
        (*m_writer)->EnableLineSeparators();
    }
    
    // Write the current element and all its children to the writer
    AddToWriter(this);
    
    
    xmlString = (*m_writer)->GetText();
    
    // delete the XML writer
    delete m_writer;
    m_writer = NULL;
}

void XElement::Load(const std::string& xmlString, XElementPtr& element)
{
    if (xmlString.empty())
    {
        throw XmlException(XElement::EXCEPTION_MESSAGE_INPUT_EMPTY);
    }
    
    xmlcxx::XMLReader reader(xmlString.c_str());
    xmlcxx::XMLElement parseElement;
    
    // Create a stack of ElementPtr
    std::stack<XElementPtr> elementStack;
    
    XElementPtr currentElement;
    while(reader.GetNext(parseElement))
    {
        if (parseElement.GetType() ==  xmlcxx::XMLElement::START)
        {
            size_t attributeCount;
    
            // if current element is not NULL, then this is a child element
            // push the current element to the stack
            if (currentElement != NULL)
            {
                elementStack.push(currentElement);
            }
            
            // Create the ElementPtr
            currentElement = new XElement(parseElement.GetData());
            
            // Loop through the attributes and add them
            attributeCount = parseElement.GetAttributeCount();
            
            for (size_t i = 0; i < attributeCount; ++i)
            {
                currentElement->SetAttributeValue(parseElement.GetAttributeName(i),
                                                parseElement.GetAttributeValue(i));
            }
        }
        
        if (parseElement.GetType() ==  xmlcxx::XMLElement::CHARS)
        {
            // If Chars was found add it as content to the current element
            currentElement->SetContent(parseElement.GetData());
        }
        
        if (parseElement.GetType() ==  xmlcxx::XMLElement::END)
        {
            // The end tag should match to the current tag. The error condition should
            // have been caught by xmlcxx. In case it is not, it would be a bug on that
            // library. So setting an assert
            assert(currentElement->GetName().compare(parseElement.GetData()) == 0);
            
            // The current element is complete. The top of the stack contains the parent
            // If the stack is empty then the current element is the root
            if (!elementStack.empty())
            {
                // Get the parent Element
                XElementPtr parentElement = elementStack.top();
                
                // Add the current element as a child
                parentElement->AddChild(currentElement);
                
                // Set the current element as the parent
                currentElement = parentElement;
                
                // Pop the stack to remove the parent element
                elementStack.pop();
            }
        }
    }
    
    //Check for errors if any throw exception
    if (reader.GetError())
    {
        throw XmlException(reader.GetErrorMessage());
    }
    
    // The currentElement is the root element. return it
    element = currentElement;
}


